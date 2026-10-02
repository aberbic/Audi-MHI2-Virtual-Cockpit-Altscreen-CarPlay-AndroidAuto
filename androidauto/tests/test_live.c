/* Host tests for nonblocking frame handoff; no vehicle libraries involved. */
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
struct endpoint { int opened; };
static struct endpoint video;
static int setup_done,main_focused;
static void project_if_ready(void) {}
static void note(const char *s,...) { (void)s; }
static int send_message(struct endpoint *e,unsigned type,const unsigned char *p,size_t n) { (void)e;(void)type;(void)p;(void)n;return 0; }
#include "aa_live.h"
int main(void)
{
    int pair[2],small=4096;unsigned char h[8],payload[64],scratch;
    unsigned char config[]={0,0,0,1,0x67,1,2,3};
    unsigned char idr[]={0,0,0,1,0x65,4,5,6};
    unsigned char predicted[]={0,0,0,1,0x41,4,5,6};
    unsigned char *big=malloc(1024*1024);
    assert(big);signal(SIGPIPE,SIG_IGN);
    assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);
    fcntl(pair[0],F_SETFL,O_NONBLOCK);fcntl(pair[1],F_SETFL,O_NONBLOCK);
    live_client=pair[0];live_config(config,sizeof(config));
    live_frame(predicted,sizeof(predicted));assert(read(pair[1],&scratch,1)==-1 && errno==EAGAIN);
    live_frame(idr,sizeof(idr));assert(live_count==1);live_drain();assert(read(pair[1],h,8)==8 && mu1438_au_length(h)==16);
    assert(read(pair[1],payload,16)==16 && !memcmp(payload,config,8) && !memcmp(payload+8,idr,8));
    live_frame(predicted,sizeof(predicted));live_drain();assert(read(pair[1],h,8)==8 && mu1438_au_length(h)==8);
    assert(read(pair[1],payload,8)==8 && !memcmp(payload,predicted,8));
    assert(live_sent==2);
    setsockopt(live_client,SOL_SOCKET,SO_SNDBUF,&small,sizeof(small));
    memset(big,1,1024*1024);live_frame(big,1024*1024);
    assert(live_count==1);assert(live_drain()==1);assert(live_count==1&&live_client>=0);
    {
        unsigned char recvbuf[8192];size_t received=0;unsigned loops=0;
        while(received<1024*1024+8 && loops++<10000){
            ssize_t n;live_drain();n=read(pair[1],recvbuf,sizeof(recvbuf));
            if(n>0)received+=(size_t)n;else assert(errno==EAGAIN);
        }
        assert(received==1024*1024+8 && live_count==0 && live_sent==3);
    }
    {
        unsigned i;
        for(i=0;i<LIVE_QUEUE_COUNT;i++)live_frame(predicted,sizeof(predicted));
        assert(live_count==LIVE_QUEUE_COUNT);live_frame(predicted,sizeof(predicted));
        assert(live_client==-1 && live_wait_idr && !live_count && !live_bytes);
    }
    close(pair[1]);free(big);
    puts("PASS: fresh IDR, config prefix, complete frames across partial writes, 1MiB frame with 4KiB socket buffer, bounded queue overflow");
    return 0;
}
