#define MU1438_HOST_TEST 1
#define main runtime_main_unused
#include "cluster_runtime.c"
#undef main
#include <assert.h>
#include <pthread.h>

static int sockets[2];
static const size_t sizes[]={1,65535,65536,65537,300000,MU1438_AU_MAX};
static void *sender(void *arg)
{
    unsigned i; (void)arg;
    for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++){
        unsigned char h[8],*payload=malloc(sizes[i]);size_t at;
        assert(payload);memset(payload,(int)(i+1),sizes[i]);mu1438_au_header(h,(uint32_t)sizes[i]);
        /* Split header into individual bytes; split data independently of AUs. */
        for(at=0;at<8;at++)assert(write(sockets[1],h+at,1)==1);
        for(at=0;at<sizes[i];){size_t n=(at%8191)+1;if(n>sizes[i]-at)n=sizes[i]-at;
            ssize_t sent=write(sockets[1],payload+at,n);assert(sent>0);at+=(size_t)sent;}
        free(payload);
    }
    close(sockets[1]);return NULL;
}
int main(void)
{
    unsigned i;pthread_t thread;unsigned char h[8];
    owner=getpid();assert(socketpair(AF_UNIX,SOCK_STREAM,0,sockets)==0);
    assert(pthread_create(&thread,NULL,sender,NULL)==0);
    for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++){
        unsigned char *payload=malloc(sizes[i]);size_t j;
        assert(read_exact_frame(sockets[0],h,8)==1);
        assert(mu1438_au_length(h)==sizes[i]);
        assert(read_exact_frame(sockets[0],payload,sizes[i])==1);
        for(j=0;j<sizes[i];j++)assert(payload[j]==i+1);
        free(payload);
    }
    assert(read_exact_frame(sockets[0],h,8)==0);close(sockets[0]);pthread_join(thread,NULL);
    mu1438_au_header(h,0);assert(!mu1438_au_length(h));
    mu1438_au_header(h,MU1438_AU_MAX+1);assert(!mu1438_au_length(h));
    memset(h,0,sizeof(h));assert(!mu1438_au_length(h));
    puts("PASS: exact AU boundaries under fragmented TCP, >64KiB frames, maximum size, EOF, invalid headers");
    return 0;
}
