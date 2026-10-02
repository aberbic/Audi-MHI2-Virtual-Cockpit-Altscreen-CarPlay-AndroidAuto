/* One-shot request through the stock iAP2 client API on an authenticated session.
 * Does not replace/start mm-ipod, authenticate, or touch display state. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
int main(int argc,char **argv){
    void *lib,*client;int rc,saved;
    void *(*connect_client)(const char *,int);
    int (*disconnect_client)(void *);
    int (*send_message)(void *,const void *,unsigned);
    static const uint8_t request[]={0x40,0x40,0,24,0x52,0,0,6,0,0,0,16,0,4,0,1,0,4,0,2,0,4,0,3};
    if(argc!=2 || strcmp(argv[1],"--send-once"))return 2;
    alarm(5);
    lib=dlopen("/mnt/app/armle/usr/lib/libiap2client.so.1",RTLD_NOW|RTLD_LOCAL);
    if(!lib){fprintf(stderr,"Client library unavailable: %s\n",dlerror());return 3;}
    connect_client=(void*(*)(const char*,int))dlsym(lib,"iap2_connect");
    disconnect_client=(int(*)(void*))dlsym(lib,"iap2_disconnect");
    send_message=(int(*)(void*,const void*,unsigned))dlsym(lib,"iap2_raw_msg_send");
    if(!connect_client || !disconnect_client || !send_message)return 4;
    /* /dev/ipod0 is the resource-manager mount root (a directory). The stock
     * control protocol uses MsgSend on its read-only descriptor, not write(). */
    client=connect_client("/dev/ipod0",O_RDONLY);
    if(!client){perror("iap2_connect");return 5;}
    rc=send_message(client,request,sizeof(request));saved=errno;
    disconnect_client(client);
    printf("One guidance subscription request: rc=%d errno=%d\n",rc,rc?saved:0);
    return rc?6:0;
}
