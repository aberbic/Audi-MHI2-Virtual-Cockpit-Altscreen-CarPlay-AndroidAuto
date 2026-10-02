/* Exact MU1438-only capability negotiation and local metadata observation.
 * User approved road/instruction text capture; never publish these logs.
 * No inline code patching, no cluster publication, no raw packet logging.
 * Opt-in only at process start; clear marker and reconnect to return to probe.
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include "rgd_wire.h"
#include "rgd_monitor.h"
#include "rgd_packet.h"
#ifndef RGD_ENABLE_MARKER
#define RGD_ENABLE_MARKER "/mnt/app/root/mu1438-rgd/metadata.enabled"
#endif
#define RGD_LOG "/tmp/mu1438-rgd-metadata.log"
typedef char pkt_storage_offset[(offsetof(struct rgd_native_packet,storage)==24)?1:-1];
typedef char pkt_control_offset[(offsetof(struct rgd_native_packet,control)==36)?1:-1];
typedef char pkt_length_offset[(offsetof(struct rgd_native_packet,payload_length)==40)?1:-1];
typedef int (*init_fn)(void *,char *);
typedef int (*ident_fn)(void *,struct rgd_native_packet *);
typedef int (*read_fn)(void *,void *,unsigned,int,unsigned);
typedef int (*send_fn)(void *,const void *,unsigned);
typedef int (*packet_fn)(void *);
struct ident_entry {uint32_t id;ident_fn callback;};
static init_fn original_init;
static ident_fn original_ident;
static read_fn original_read;
static send_fn send_raw;
static packet_fn original_packet;
static struct rgd_monitor monitor;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static int transport_ready,advertised,accepted,authenticated,sent,sending,rejected;
static unsigned attempts,events;

static void log_line(const char *line){
    int saved=errno,fd;char prefix[40];int length;
    if(events++>=4000){errno=saved;return;}
    fd=open(RGD_LOG,O_WRONLY|O_CREAT|O_APPEND,0600);
    if(fd>=0){length=snprintf(prefix,sizeof(prefix),"pid=%ld ",(long)getpid());write(fd,prefix,(size_t)length);write(fd,line,strlen(line));write(fd,"\n",1);close(fd);}errno=saved;
}
static void received(void *unused,const uint8_t *p,size_t n){
    unsigned msg=((unsigned)p[4]<<8)|p[5];struct rgd_event e;char line[2048];int len;unsigned i;
    (void)unused;
    if(msg==0x1d02){accepted=1;log_line("identification accepted; subscription pending");return;}
    if(msg==0x1d03){accepted=0;advertised=0;rejected=1;log_line("identification rejected; next identification uses stock capabilities, no subscription");return;}
    if(msg==0xaa05){authenticated=1;log_line("stock accessory authentication succeeded; guidance request permitted");return;}
    if(msg==0xaa04){authenticated=0;log_line("stock accessory authentication failed; no guidance request");return;}
    if(msg<0x5200 || msg>0x5204)return;
    if(rgd_parse(p,n,&e)!=1){log_line("guidance message rejected by recorder validation; stock bytes unchanged");return;}
    len=snprintf(line,sizeof(line),"guidance msg=0x%04x bytes=%u present=%08x",msg,(unsigned)n,e.present);
    for(i=0;i<32 && len>0 && (unsigned)len<sizeof(line)-80;i++)if(e.present&(1u<<i)){
        if(e.numeric&(1u<<i))len+=snprintf(line+len,sizeof(line)-(unsigned)len," f%u=%llu",i,(unsigned long long)e.value[i]);
        else len+=snprintf(line+len,sizeof(line)-(unsigned)len," f%u[length=%u]",i,e.length[i]);
    }
    log_line(line);
    /* Only bounded navigation strings; never dump authentication/identity data. */
    {
        size_t at;
        for(at=6;at<n;){
            unsigned size=((unsigned)p[at]<<8)|p[at+1],id=((unsigned)p[at+2]<<8)|p[at+3];
            if((msg==0x5201 && (id==3 || id==4 || id==19)) || (msg==0x5202 && (id==2 || id==4 || id==13))){
                char value[161];unsigned j=0;
                while(j<size-4 && j<sizeof(value)-1 && p[at+4+j]){unsigned c=p[at+4+j];value[j]=(char)(c<32?32:c);j++;}
                value[j]=0;snprintf(line,sizeof(line),"guidance text msg=0x%04x field=%u value=%s",msg,id,value);log_line(line);
            }
            at+=size;
        }
    }
}
static int capture_read(void *device,void *buffer,unsigned capacity,int timeout,unsigned flags){
    int rc=original_read(device,buffer,capacity,timeout,flags),saved=errno;
    if(rc>0 && (unsigned)rc<=capacity){
        pthread_mutex_lock(&lock);rgd_monitor_feed(&monitor,buffer,(size_t)rc);pthread_mutex_unlock(&lock);
    }
    errno=saved;return rc;
}
static int receive_packet(void *context){
    int result=original_packet(context),saved=errno,subscribe=0,rc;unsigned attempt=0;
    /* The raw-read callback executes under a stock link lock. Never transmit
     * there. Wait until the complete stock receive/dispatch callback returns. */
    pthread_mutex_lock(&lock);
    if(result>=0 && transport_ready && advertised && accepted && authenticated && !sent && !sending && attempts<3){
        attempt=++attempts;sending=1;subscribe=1;
    }
    pthread_mutex_unlock(&lock);
    if(subscribe){
        static const uint8_t request[]={0x40,0x40,0,24,0x52,0,0,6,0,0,0,16,0,4,0,1,0,4,0,2,0,4,0,3};
        char line[96];rc=send_raw(context,request,sizeof(request));
        pthread_mutex_lock(&lock);sending=0;if(rc==0)sent=1;
        snprintf(line,sizeof(line),"subscription request rc=%d attempt=%u",rc,attempt);log_line(line);pthread_mutex_unlock(&lock);
    }
    errno=saved;return result;
}
static int identify(void *context,struct rgd_native_packet *packet){
    int rc=original_ident(context,packet),saved=errno,allow;
    pthread_mutex_lock(&lock);allow=!rejected;pthread_mutex_unlock(&lock);
    if(rc==0 && transport_ready && allow){
        int changed=rgd_packet_identify(packet,16);
        pthread_mutex_lock(&lock);advertised=changed;
        log_line(changed?"route-guidance capability appended; original identity retained":"capability append refused; stock identification retained");pthread_mutex_unlock(&lock);
    } else if(rc==0 && !allow){
        pthread_mutex_lock(&lock);advertised=0;log_line("fallback: original stock identification retained");pthread_mutex_unlock(&lock);
    }
    errno=saved;return rc;
}
static int initialize(void *context,char *options){
    void *device;uintptr_t *transport;Dl_info info;read_fn candidate;int rc;
    memcpy(&device,(uint8_t*)context+20,sizeof(device));
    if(!device)return original_init(context,options);
    memcpy(&transport,(uint8_t*)device+64,sizeof(transport));
    if(!transport || !transport[0] || strcmp((const char*)transport[0],"ipod_transport"))return original_init(context,options);
    candidate=(read_fn)transport[11];memset(&info,0,sizeof(info));
    if(!candidate || !dladdr((void*)candidate,&info) || !info.dli_fname || !strstr(info.dli_fname,"ipod-transport-usbdevice.so")){
        log_line("unsupported transport; metadata disabled");return original_init(context,options);
    }
    original_read=candidate;
    rgd_monitor_init(&monitor,received,NULL);
    transport[11]=(uintptr_t)capture_read;transport_ready=1;
    log_line("USB receive observer installed; data and return values delegated unchanged");
    rc=original_init(context,options);
    if(rc!=0){int saved=errno;transport[11]=(uintptr_t)candidate;transport_ready=0;log_line("native initialization failed; observer removed");errno=saved;}
    return rc;
}
void rgd_install(void *handle,uintptr_t *driver){
    struct ident_entry *table;
    if(access(RGD_ENABLE_MARKER,F_OK)!=0)return;
    table=(struct ident_entry*)dlsym(handle,"ident_info_funcs");
    original_init=(init_fn)dlsym(handle,"iap2_init");send_raw=(send_fn)dlsym(handle,"link_send_raw_message");
    original_packet=(packet_fn)dlsym(handle,"transport_recv_pkt");
    if(!table || !original_init || !send_raw || !original_packet || driver[6]!=(uintptr_t)original_init || driver[8]!=(uintptr_t)original_packet || table[23].id!=24 || !table[23].callback || table[6].id!=6 || table[7].id!=7){
        log_line("native ABI mismatch; metadata disabled");return;
    }
    original_ident=table[23].callback;
    table[23].callback=identify;driver[6]=(uintptr_t)initialize;driver[8]=(uintptr_t)receive_packet;
    log_line("opt-in adapter installed; cluster output disabled");
}
