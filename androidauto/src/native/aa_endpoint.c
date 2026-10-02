/* MU1438 endpoint-based capture prototype, NOT a cluster renderer.
 * Preserve every stock endpoint. Additional services use their own callback
 * tables and the channel negotiated by stock; no router/video/audio hooks.
 * Stock protobuf drops modern cluster fields: append only after serialization.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if defined(AA_RELEASE) && !defined(AA_ENDPOINT_HOST_TEST)
#include <process.h>
#endif
#include "aa_wire.h"
#include "aa_state.h"

struct endpoint {
    void **vtable;
    unsigned char opened,channel,pad1[2];
    void *router;
    unsigned char service,pad2[3],raw,pad3[3];
    uint32_t session;
};
struct io_buffer { unsigned char *data;uint32_t capacity,read_pos,write_pos; };
struct shared_buffer { void *owner;struct io_buffer *buffer; };
#ifndef AA_ENDPOINT_HOST_TEST
typedef char check_endpoint[offsetof(struct endpoint,service)==12 && offsetof(struct endpoint,raw)==16?1:-1];
typedef char check_io[sizeof(struct io_buffer)==16?1:-1];
#endif
static int (*real_register)(void *,void *);
static int (*real_marshall)(void *,unsigned short,const void *,struct io_buffer *);
static int (*queue_out)(void *,unsigned char,void *,unsigned);
static int (*real_focus)(void *,int,unsigned char);
static int (*real_prepare_shutdown)(void *);
static void *(*new_array)(size_t);
static void (*delete_array)(void *);
static pthread_once_t once=PTHREAD_ONCE_INIT;
static pthread_mutex_t log_lock=PTHREAD_MUTEX_INITIALIZER;
static int log_fd=-1,capture_fd=-1,registered,setup_done,main_focused,focus_sent;
static void *main_sink;
static unsigned long frames,capture_bytes;
static void *callbacks[8];
static struct endpoint video,input;
static void project_if_ready(void);

static void note(const char *fmt,...)
{
    char b[600];int n;va_list ap;va_start(ap,fmt);n=vsnprintf(b,sizeof(b)-2,fmt,ap);va_end(ap);
    if(n<0)return;if(n>(int)sizeof(b)-2)n=sizeof(b)-2;b[n++]='\n';
    pthread_mutex_lock(&log_lock);if(log_fd>=0)(void)write(log_fd,b,n);pthread_mutex_unlock(&log_lock);
}
#if AA_DIAGNOSTICS
#include "aa_diagnostics.h"
#endif
static void resolve(void)
{
#define GET(dst,name) (*(void **)(&(dst))=dlsym(RTLD_NEXT,name))
    GET(real_register,"_ZN11GalReceiver15registerServiceEP20ProtocolEndpointBase");
    GET(real_marshall,"_ZN13MessageRouter13marshallProtoEtRKN6google8protobuf11MessageLiteEP8IoBuffer");
    GET(queue_out,"_ZN13MessageRouter13queueOutgoingEhPvj");
    GET(real_focus,"_ZN9VideoSink13setVideoFocusEib");
    GET(real_prepare_shutdown,"_ZN11GalReceiver15prepareShutdownEv");
    *(void **)(&new_array)=dlsym(RTLD_DEFAULT,"_Znaj");
    *(void **)(&delete_array)=dlsym(RTLD_DEFAULT,"_ZdaPv");
    note("resolved register=%p marshall=%p outgoing=%p focus=%p new[]=%p delete[]=%p",real_register,real_marshall,queue_out,real_focus,new_array,delete_array);
}
static int send_message(struct endpoint *e,unsigned type,const unsigned char *data,size_t n)
{
    unsigned char packet[80];int rc;
    if(!e->opened || !queue_out || n>sizeof(packet)-2)return 0;
    packet[0]=(unsigned char)(type>>8);packet[1]=(unsigned char)type;memcpy(packet+2,data,n);
    rc=queue_out(e->router,e->channel,packet,(unsigned)n+2);
    if(type!=0x8004)note("send service=%u channel=%u type=0x%x rc=%d",e->service,e->channel,type,rc);
    return rc;
}
#ifdef AA_LIVE
#include "aa_live.h"
#endif
#if AA_ZOOM
#include "aa_input.h"
#endif
static void project_if_ready(void)
{
    static const unsigned char focus[]={8,1,16,0};
#ifdef AA_LIVE
    int connected;
    pthread_mutex_lock(&live_lock);connected=live_client>=0;pthread_mutex_unlock(&live_lock);
    if(!connected)return; /* Decoder must be ready before the sole initial IDR. */
#endif
    if(setup_done && main_focused && video.opened && !focus_sent){
        focus_sent=1;send_message(&video,0x8008,focus,sizeof(focus));
        note("cluster focus requested after main focus");
    }
}
static void stream_state(int ready)
{
#ifdef AA_LIVE
    static int previous=-1;
    if(previous==ready)return;
    if(aa_state_write("/tmp/aa_endpoint.state",ready)==0){previous=ready;note("live readiness=%d published",ready);}
#else
    (void)ready;
#endif
}
static void destroy_endpoint(struct endpoint *e) { (void)e; }
static int closed(struct endpoint *e,unsigned char channel)
{
#if AA_ZOOM
    if(e==&input || e==&video)input_enable(0);
#endif
    int was=e->opened;e->opened=0;
    if(e==&video){setup_done=0;focus_sent=0;stream_state(0);}
    note("closed service=%u channel=%u",e->service,channel);return was;
}
static int may_open(struct endpoint *e,unsigned char channel) { (void)channel;return !e->opened; }
static void opened(struct endpoint *e,unsigned char channel)
{
    e->opened=1;e->channel=channel;e->session=0;
    note("opened service=%u negotiatedChannel=%u",e->service,channel);
}
static void discovery(struct endpoint *e,void *response) { (void)e;(void)response; }
static int raw_message(struct endpoint *e,unsigned char ch,const void *buf) { (void)e;(void)ch;(void)buf;return -253; }
static void capture(const unsigned char *p,size_t n)
{
    if(capture_fd<0 || capture_bytes>=8u*1024u*1024u)return;
    if(n>8u*1024u*1024u-capture_bytes)n=8u*1024u*1024u-capture_bytes;
    if(write(capture_fd,p,n)!=(ssize_t)n){close(capture_fd);capture_fd=-1;note("capture stopped: write failed");return;}
    capture_bytes+=n;
}
static int route(struct endpoint *e,unsigned char channel,unsigned short type,const struct shared_buffer *shared)
{
    const struct io_buffer *b;const unsigned char *p;size_t n;uint32_t session=0;
    if(!shared || !(b=shared->buffer) || !b->data || b->capacity>4u*1024u*1024u || b->read_pos>b->write_pos || b->write_pos>b->capacity || b->write_pos-b->read_pos<2)return -253;
    p=b->data+b->read_pos+2;n=b->write_pos-b->read_pos-2;
    if(!e->opened || channel!=e->channel)return -253;
    if(type>1)note("receive service=%u channel=%u type=0x%x bytes=%u",e->service,channel,type,(unsigned)n);
    if(e==&input){
        if(type==0x8002){
            static const unsigned char ok[]={8,0};send_message(e,0x8003,ok,sizeof(ok));
#if AA_ZOOM
            input_enable(real_prepare_shutdown!=NULL);note("AA cluster key binding ready=%d",real_prepare_shutdown!=NULL);
#endif
        }
        return 0;
    }
    switch(type){
    case 0x8000: {
        static const unsigned char config[]={8,2,16,1,24,0};
        send_message(e,0x8003,config,sizeof(config));setup_done=1;
        /* Dimensions are advertised/known. Let NvSS initialize and attach
         * BEFORE requesting projection; waiting for SPS/PPS loses the IDR. */
        stream_state(1);project_if_ready();break;
    }
    case 0x8001:
        if(aa_getint(p,n,1,&session)!=1)return -253;
        e->session=session;note("cluster started session=%u",session);break;
    case 0x8002:focus_sent=0;stream_state(0);note("cluster stream stopped");break;
    case 0x8007:focus_sent=0;project_if_ready();break;
    case 0x0001:
        capture(p,n);note("cluster codec config bytes=%u",(unsigned)n);
#ifdef AA_LIVE
        live_config(p,n);
        stream_state(1);
#endif
        break;
    case 0x0000: {
        unsigned char ack[24];struct aa_pb msg={ack,0,sizeof(ack),0};
        if(n<8)return -253;
        capture(p+8,n-8);frames++;
#if AA_DIAGNOSTICS
        diag_frame(p,n-8);
#endif
#ifdef AA_LIVE
        live_frame(p+8,n-8);
        stream_state(1);
#endif
        aa_int(&msg,1,e->session);aa_int(&msg,2,1);
        if(!msg.bad)send_message(e,0x8004,msg.data,msg.used);
        if(frames==1 || frames%120==0)note("cluster frame=%lu channel=%u bytes=%u captured=%lu",frames,channel,(unsigned)n-8,capture_bytes);
        break;
    }
    default:break;
    }
    return 0;
}
static void init_endpoint(struct endpoint *e,void *receiver,unsigned service)
{
    memset(e,0,sizeof(*e));e->vtable=callbacks;e->router=receiver;e->service=(unsigned char)service;e->channel=255;
}
int _ZN11GalReceiver15registerServiceEP20ProtocolEndpointBase(void *receiver,void *ep)
{
    int rc;pthread_once(&once,resolve);if(!real_register)return 0;
    rc=real_register(receiver,ep);
    if(rc && !registered && ep && ((unsigned char *)ep)[12]==1 && real_marshall && queue_out && new_array && delete_array){
        Dl_info info;void **vt=*(void ***)ep;memset(&info,0,sizeof(info));
        if(!vt || !dladdr(vt,&info) || !info.dli_sname || strcmp(info.dli_sname,"_ZTV9VideoSink"))return rc;
        main_sink=ep;
        callbacks[0]=callbacks[1]=(void *)destroy_endpoint;callbacks[2]=(void *)closed;callbacks[3]=(void *)may_open;
        callbacks[4]=(void *)opened;callbacks[5]=(void *)route;callbacks[6]=(void *)raw_message;callbacks[7]=(void *)discovery;
        init_endpoint(&video,receiver,19);init_endpoint(&input,receiver,20);
        registered=real_register(receiver,&video) && real_register(receiver,&input);
        note("cluster endpoints registered=%d; main sink=%p unchanged",registered,main_sink);
    }
    return rc;
}
int _ZN13MessageRouter13marshallProtoEtRKN6google8protobuf11MessageLiteEP8IoBuffer(void *router,unsigned short type,const void *message,struct io_buffer *out)
{
    int rc;unsigned char extra[128],*replacement;size_t n;unsigned old;
    pthread_once(&once,resolve);if(!real_marshall)return 0;
    rc=real_marshall(router,type,message,out);
    if(type!=6 || !registered || !out || !out->data || out->read_pos || out->write_pos<2 || out->write_pos>65536 || out->write_pos>out->capacity)return rc;
    if(out->data[0]!=0 || out->data[1]!=6)return rc;
#if AA_DIAGNOSTICS
    diag_discovery(out->data+2,out->write_pos-2);
#endif
    n=aa_cluster_services(extra,sizeof(extra));if(!n)return rc;
    old=out->write_pos;replacement=new_array(old+n);if(!replacement)return rc;
    memcpy(replacement,out->data,old);memcpy(replacement+old,extra,n);
    delete_array(out->data);out->data=replacement;out->capacity=out->write_pos=old+(unsigned)n;
    note("SDR appended after serialization: stock=%u extra=%u total=%u",old,(unsigned)n,out->write_pos);
    return rc;
}
int _ZN9VideoSink13setVideoFocusEib(void *sink,int mode,unsigned char unsolicited)
{
    int rc;pthread_once(&once,resolve);if(!real_focus)return 0;
    rc=real_focus(sink,mode,unsolicited);
    if(sink==main_sink){if(mode==1)main_focused=1;note("main focus=%d rc=%d",mode,rc);project_if_ready();}
    return rc;
}
#if AA_ZOOM
int _ZN11GalReceiver15prepareShutdownEv(void *receiver)
{
    pthread_once(&once,resolve);input_enable(0);stream_state(0);
    return real_prepare_shutdown?real_prepare_shutdown(receiver):0;
}
#endif
#ifndef AA_ENDPOINT_HOST_TEST
__attribute__((constructor)) static void begin(void)
{
    log_fd=open("/tmp/aa_endpoint.log",O_WRONLY|O_CREAT|O_APPEND,0600);
#ifdef AA_RELEASE
    if(access("/tmp/aa_capture.enabled",F_OK)==0)
#endif
    capture_fd=open("/tmp/aa_endpoint.h264",O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(log_fd>=0)fcntl(log_fd,F_SETFD,FD_CLOEXEC);if(capture_fd>=0)fcntl(capture_fd,F_SETFD,FD_CLOEXEC);
#ifdef AA_RELEASE
    note("AA cluster release pid=%ld; capture=%s",(long)getpid(),capture_fd>=0?"enabled":"disabled");
#else
    note("AA endpoint capture prototype pid=%ld; no decoder or cluster context changes",(long)getpid());
#endif
#ifdef AA_LIVE
    stream_state(0);
    start_live();
#endif
#if AA_ZOOM
    start_input();
#endif
#ifdef AA_RELEASE
    {
        char parent[32];int child;
        snprintf(parent,sizeof(parent),"%ld",(long)getpid());
        child=spawnl(P_NOWAITO,"/bin/sh","sh","/mnt/app/root/mu1438-aa/aa_supervisor.sh",parent,(char *)NULL);
        note("AA permanent supervisor spawn=%d",child);
    }
#endif
}
#endif
