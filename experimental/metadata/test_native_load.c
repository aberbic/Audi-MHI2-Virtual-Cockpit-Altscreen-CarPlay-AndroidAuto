/* Loader-only harness. NEVER invokes the driver interface or opens a device.
 * Export the executable symbols referenced by the native module. Every stub
 * terminates immediately if unexpectedly invoked: none performs real work.
 * This cannot substitute for a functional mm-ipod test. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
char mountpoint[24];
#define HOST_EXPORT(name) void name(void) { fputs("UNEXPECTED HOST CALLBACK: " #name "\n",stderr); _exit(79); }
HOST_EXPORT(ipod_cfg_find)
HOST_EXPORT(ipod_cfg_list_init)
HOST_EXPORT(ipod_cfg_load)
HOST_EXPORT(systime_ms)
HOST_EXPORT(acp_challenge)
HOST_EXPORT(ipod_log_threadid)
HOST_EXPORT(ipod_log)
HOST_EXPORT(ipod_resmgr_mount)
HOST_EXPORT(ipod_cfg_list_get)
HOST_EXPORT(ipod_print_bytes)
int main(int argc,char **argv) {
    void *h;int flags;uintptr_t *module;
    const char *path="/mnt/app/root/mu1438-rgd/ipod-drvr-iap2.stock.so";
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"now"))flags=RTLD_NOW|RTLD_LOCAL;
    else if(!strcmp(argv[1],"factory"))flags=RTLD_GLOBAL;
    else if(!strcmp(argv[1],"probe")){flags=RTLD_GLOBAL;path="/tmp/mu1438-rgd-v2-probe.so";}
    else if(!strcmp(argv[1],"capture")){flags=RTLD_GLOBAL;path="/tmp/mu1438-rgd-capture-test.so";}
    else return 2;
    h=dlopen(path,flags);
    if(!h){const char *e=dlerror();printf("LOAD_FAILED mode=%s error=%s\n",argv[1],e?e:"<no error>");return 3;}
    module=(uintptr_t*)dlsym(h,"ipod_module");
    if(!module || (module[1]&0xffff)!=0x200 || !module[8])return 4;
    if(!strcmp(argv[1],"capture")) {
        void *stock=dlopen("/mnt/app/root/mu1438-rgd/ipod-drvr-iap2.stock.so",RTLD_GLOBAL);
        uintptr_t *driver=(uintptr_t*)module[8];
        if(!stock || driver[6]==(uintptr_t)dlsym(stock,"iap2_init") || driver[8]==(uintptr_t)dlsym(stock,"transport_recv_pkt"))return 5;
        puts("CAPTURE_BINDINGS_OK: guarded native callback substitutions installed in isolated process");
    }
    puts("LOAD_OK: stock module ABI verified; no driver callbacks invoked");
    /* Exit process without invoking interface init, teardown or callbacks. */
    return 0;
}
