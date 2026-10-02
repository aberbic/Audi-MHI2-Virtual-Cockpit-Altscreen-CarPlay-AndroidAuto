/* HMI already emits MZ01 signed steps to loopback UDP19822 while a renderer
 * heartbeat is active. This AA-only process translates those to the separately
 * advertised cluster input endpoint. CarPlay's own receiver is unchanged.
 * /tmp/aa_zoom_mode beginning with 'r' selects a rotary event for app comparison;
 * otherwise send dedicated ZOOM_IN/ZOOM_OUT down/up pairs. */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
static pthread_mutex_t input_lock=PTHREAD_MUTEX_INITIALIZER;
static int input_ready;
static void input_enable(int enabled)
{
    pthread_mutex_lock(&input_lock);input_ready=enabled;pthread_mutex_unlock(&input_lock);
}
static uint64_t input_micros(void)
{
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
    return (uint64_t)t.tv_sec*1000000u+t.tv_nsec/1000u;
}
static void *input_worker(void *unused)
{
    int fd;struct sockaddr_in a;(void)unused;
    fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)return NULL;
    memset(&a,0,sizeof(a));a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);a.sin_port=htons(19822);
    if(bind(fd,(struct sockaddr *)&a,sizeof(a))<0){note("AA zoom bind failed errno=%d",errno);close(fd);return NULL;}
    fcntl(fd,F_SETFD,FD_CLOEXEC);note("AA zoom input listening on loopback UDP19822");
    for(;;){
        unsigned char wire[9],report[72];uint32_t net;int32_t steps;ssize_t got;struct stat st;
        int rotary=0,mode_fd,count,i;char mode=0;
        got=recv(fd,wire,sizeof(wire),0);if(got<0){if(errno==EINTR)continue;break;}
#ifdef AA_RELEASE
        /* Both tested event formats were ignored by the user's app. Keep
         * unverified forwarding opt-in, without changing CarPlay input. */
        if(access("/tmp/aa_zoom.enabled",F_OK)!=0)continue;
#endif
        if(got!=8 || memcmp(wire,"MZ01",4))continue;
        memcpy(&net,wire+4,4);steps=(int32_t)ntohl(net);if(!steps || steps< -32 || steps>32)continue;
        if(stat("/tmp/mu1438-cluster-active",&st)!=0 || time(NULL)<st.st_mtime || time(NULL)-st.st_mtime>2)continue;
        mode_fd=open("/tmp/aa_zoom_mode",O_RDONLY);
        if(mode_fd>=0){if(read(mode_fd,&mode,1)==1)rotary=mode=='r';close(mode_fd);}
        pthread_mutex_lock(&input_lock);
        if(input_ready && input.opened && video.opened){
            size_t n;note("AA zoom steps=%d mode=%s channel=%u",steps,rotary?"rotary":"zoom-keys",input.channel);
            count=rotary?1:(steps<0?-steps:steps);
            for(i=0;i<count;i++){
                n=aa_zoom_report(report,sizeof(report),input_micros(),steps,rotary,1);
                if(n)send_message(&input,0x8001,report,n);
                if(!rotary){n=aa_zoom_report(report,sizeof(report),input_micros(),steps,0,0);if(n)send_message(&input,0x8001,report,n);}
            }
        }
        pthread_mutex_unlock(&input_lock);
    }
    close(fd);return NULL;
}
static void start_input(void)
{
    pthread_t thread;if(pthread_create(&thread,NULL,input_worker,NULL)==0)pthread_detach(thread);
    else note("AA zoom input thread failed");
}
