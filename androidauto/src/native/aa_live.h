/* Optional capture-to-render handoff. No socket I/O on AA's inbound thread.
 * A bounded worker queue handles partial writes (QNX socket buffer ~64KiB). The new
 * consumer receives cached SPS/PPS plus a freshly requested IDR, never old P
 * pictures. Phone ACKs remain independent from renderer delivery.
 */
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include "mu1438_au_wire.h"
static pthread_mutex_t live_lock=PTHREAD_MUTEX_INITIALIZER;
static int live_client=-1,live_wait_idr=1;
static unsigned char live_codec[65536];
static size_t live_codec_size;
static unsigned long live_sent,live_reconnects;
struct live_packet { unsigned char *data;size_t length,offset; };
#define LIVE_QUEUE_COUNT 16
#define LIVE_QUEUE_BYTES (8u*1024u*1024u)
static struct live_packet live_packets[LIVE_QUEUE_COUNT];
static unsigned live_head,live_count;
static size_t live_bytes;
static void live_drop(void)
{
    unsigned i;
    if(live_client>=0){shutdown(live_client,SHUT_RDWR);close(live_client);}
    live_client=-1;live_wait_idr=1;
    for(i=0;i<LIVE_QUEUE_COUNT;i++){free(live_packets[i].data);memset(&live_packets[i],0,sizeof(live_packets[i]));}
    live_head=live_count=0;live_bytes=0;
}
static int live_has_idr(const unsigned char *p,size_t n)
{
    size_t i;
    for(i=0;i+4<n;i++)if(!p[i]&&!p[i+1] &&
        ((p[i+2]==1&&(p[i+3]&31)==5)||(!p[i+2]&&p[i+3]==1&&(p[i+4]&31)==5)))return 1;
    return 0;
}
static void live_config(const unsigned char *p,size_t n)
{
    pthread_mutex_lock(&live_lock);
    if(n<=sizeof(live_codec)){
        if(live_codec_size && (n!=live_codec_size || memcmp(p,live_codec,n)))live_drop();
        memcpy(live_codec,p,n);live_codec_size=n;
    }else {live_codec_size=0;live_drop();}
    pthread_mutex_unlock(&live_lock);
}
static void live_frame(const unsigned char *p,size_t n)
{
    struct live_packet *packet;unsigned char *copy;size_t prefix,total;
    pthread_mutex_lock(&live_lock);
    if(live_client<0 || !live_codec_size || (live_wait_idr&&!live_has_idr(p,n)))goto done;
    prefix=live_wait_idr?live_codec_size:0;
    if(n>MU1438_AU_MAX-prefix){live_drop();goto done;}
    total=8+prefix+n;
    if(live_count==LIVE_QUEUE_COUNT || total>LIVE_QUEUE_BYTES-live_bytes){note("live queue full count=%u bytes=%u",live_count,(unsigned)live_bytes);live_drop();goto done;}
    copy=malloc(total);if(!copy){live_drop();goto done;}
    mu1438_au_header(copy,(uint32_t)(prefix+n));
    memcpy(copy+8,live_codec,prefix);memcpy(copy+8+prefix,p,n);
    packet=&live_packets[(live_head+live_count)%LIVE_QUEUE_COUNT];
    packet->data=copy;packet->length=total;packet->offset=0;
    live_count++;live_bytes+=total;live_wait_idr=0;
done:
    pthread_mutex_unlock(&live_lock);
}
/* Exactly one worker calls this. Keep frame and header offsets across EAGAIN. */
static int live_drain(void)
{
    int progress=0;ssize_t sent;struct live_packet *packet;
    pthread_mutex_lock(&live_lock);
    if(live_client<0 || !live_count)goto done;
    packet=&live_packets[live_head];
    sent=send(live_client,packet->data+packet->offset,packet->length-packet->offset,MSG_NOSIGNAL);
    if(sent>0){
        packet->offset+=(size_t)sent;progress=1;
        if(packet->offset==packet->length){
            live_bytes-=packet->length;free(packet->data);memset(packet,0,sizeof(*packet));
            live_head=(live_head+1)%LIVE_QUEUE_COUNT;live_count--;live_sent++;
            if(live_sent==1 || live_sent%120==0)note("live frames=%lu queue=%u",live_sent,live_count);
        }
    }else if(sent==0 || (errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)){
        note("live socket closed/error errno=%d",sent<0?errno:0);live_drop();
    }
done:
    pthread_mutex_unlock(&live_lock);return progress;
}
static void *live_accept(void *unused)
{
    int listener,one=1;struct sockaddr_in a;(void)unused;
    listener=socket(AF_INET,SOCK_STREAM,0);if(listener<0)return NULL;
    setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
    memset(&a,0,sizeof(a));a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);a.sin_port=htons(19840);
    if(bind(listener,(struct sockaddr *)&a,sizeof(a))<0 || listen(listener,1)<0){note("live bind/listen failed errno=%d",errno);close(listener);return NULL;}
    if(fcntl(listener,F_SETFL,fcntl(listener,F_GETFL,0)|O_NONBLOCK)<0){close(listener);return NULL;}
    note("live AU11 handoff listening on loopback:19840");
    for(;;){
        int fd=accept(listener,NULL,NULL),buffer=65536;
        if(fd>=0){
            int flags=fcntl(fd,F_GETFL,0);
            if(flags<0 || fcntl(fd,F_SETFL,flags|O_NONBLOCK)<0 || setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&one,sizeof(one))<0){close(fd);continue;}
            setsockopt(fd,SOL_SOCKET,SO_SNDBUF,&buffer,sizeof(buffer));fcntl(fd,F_SETFD,FD_CLOEXEC);
            pthread_mutex_lock(&live_lock);live_drop();live_client=fd;live_reconnects++;pthread_mutex_unlock(&live_lock);
            note("live consumer=%lu ready; cluster projection may now start",live_reconnects);
            project_if_ready();
        }
        if(!live_drain())usleep(2000);
    }
    return NULL;
}
static void start_live(void)
{
    pthread_t worker;
    if(pthread_create(&worker,NULL,live_accept,NULL)==0)pthread_detach(worker);
    else note("live thread creation failed");
}
