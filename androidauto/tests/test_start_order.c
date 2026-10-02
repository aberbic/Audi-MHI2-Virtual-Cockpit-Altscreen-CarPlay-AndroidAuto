#define _GNU_SOURCE
#define AA_ENDPOINT_HOST_TEST 1
#define AA_LIVE 1
#include <sys/socket.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#include "aa_endpoint.c"
#include <assert.h>
static int requests;
static int mock_queue(void *router,unsigned char channel,void *packet,unsigned size)
{
    const unsigned char *p=packet;(void)router;
    assert(channel==19 && size==6 && p[0]==0x80 && p[1]==8 && p[2]==8 && p[3]==1);
    requests++;return 0;
}
int main(void)
{
    int pair[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);
    queue_out=mock_queue;setup_done=1;main_focused=1;video.opened=1;video.channel=19;
    project_if_ready();assert(requests==0 && focus_sent==0);
    live_client=pair[0];project_if_ready();assert(requests==1 && focus_sent==1);
    project_if_ready();assert(requests==1);
    close(pair[0]);close(pair[1]);live_client=-1;
    puts("PASS: no cluster projection before decoder attachment; exactly one initial projection request afterward");
    return 0;
}
