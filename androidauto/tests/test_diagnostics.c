#define _GNU_SOURCE
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "aa_wire.h"
static unsigned calls;
static void note(const char *fmt,...){
    va_list ap;va_start(ap,fmt);
    if(strstr(fmt,"advertised video")){
        unsigned service=va_arg(ap,unsigned),resolution=va_arg(ap,unsigned),fps=va_arg(ap,unsigned);
        assert(service==19 && resolution==(AA_HD?3u:2u) && fps==2);calls++;
    }
    va_end(ap);
}
#include "aa_diagnostics.h"
int main(void){
    unsigned char wire[256],before[256],ts[8]={0};size_t n=aa_cluster_services(wire,sizeof(wire)),i;
    assert(n);memcpy(before,wire,n);diag_discovery(wire,n);assert(calls==1 && !memcmp(wire,before,n));
    for(i=0;i<n;i++)diag_discovery(wire,i);
    assert(!memcmp(wire,before,n));
    for(i=0;i<120;i++)diag_frame(ts,100);
    puts("PASS: read-only discovery diagnostics, truncated input bounds and timing sampler");return 0;
}
