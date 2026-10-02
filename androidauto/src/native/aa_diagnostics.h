#ifndef AA_DIAGNOSTICS_H
#define AA_DIAGNOSTICS_H
#include <time.h>
#include <sys/resource.h>
/* Read-only observers at the already-used secondary receive and SDR seams.
 * No added interposition of main media routing or input events. */
static uint64_t diag_ms(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;}
static long long diag_cpu_ms(void){struct rusage r;if(getrusage(RUSAGE_SELF,&r))return -1;return (long long)r.ru_utime.tv_sec*1000+r.ru_utime.tv_usec/1000+(long long)r.ru_stime.tv_sec*1000+r.ru_stime.tv_usec/1000;}
static void diag_frame(const unsigned char *timestamp,size_t size){
    static uint64_t began,last,bytes,ts_previous;static unsigned count;
    static long long cpu_previous;
    uint64_t now,ts=0;unsigned i;
    for(i=0;i<8;i++)ts=(ts<<8)|timestamp[i];
    if(!count){began=last=diag_ms();cpu_previous=diag_cpu_ms();ts_previous=ts;}
    count++;bytes+=size;
    if(count%120==0){
        long long cpu=diag_cpu_ms();now=diag_ms();
        note("diag cluster frames=%u elapsed_ms=%llu window_ms=%llu window_bytes=%llu gal_cpu_ms=%lld source_ts_delta=%llu",
             count,(unsigned long long)(now-began),(unsigned long long)(now-last),(unsigned long long)bytes,
             cpu<0||cpu_previous<0?-1:cpu-cpu_previous,(unsigned long long)(ts>=ts_previous?ts-ts_previous:0));
        last=now;bytes=0;cpu_previous=cpu;ts_previous=ts;
    }
}
struct diag_pb_field {unsigned number,wire;const unsigned char *bytes;size_t size;uint64_t value;};
static int diag_next(const unsigned char *p,size_t n,size_t *at,struct diag_pb_field *f){
    uint64_t tag,v;size_t end;
    if(*at==n)return 0;
    if(aa_takevar(p,n,at,&tag) || !(tag>>3) || tag>>3>0x1fffffff)return -1;
    f->number=(unsigned)(tag>>3);f->wire=(unsigned)(tag&7);f->bytes=NULL;f->size=0;f->value=0;
    if(f->wire==0){if(aa_takevar(p,n,at,&f->value))return -1;return 1;}
    if(f->wire==2){if(aa_takevar(p,n,at,&v) || v>n-*at)return -1;f->bytes=p+*at;f->size=(size_t)v;*at+=(size_t)v;return 1;}
    if(f->wire==1)end=8;else if(f->wire==5)end=4;else return -1;
    if(end>n-*at)return -1;*at+=end;return 1;
}
static void diag_discovery(const unsigned char *p,size_t n){
    size_t at=0;struct diag_pb_field service;unsigned printed=0;
    while(diag_next(p,n,&at,&service)>0){
        size_t a=0;struct diag_pb_field field;uint32_t id=0;
        if(service.number!=1 || service.wire!=2 || aa_getint(service.bytes,service.size,1,&id)!=1)continue;
        while(diag_next(service.bytes,service.size,&a,&field)>0){
            size_t b=0;struct diag_pb_field video;uint32_t type=0;
            if(field.number!=3 || field.wire!=2 || aa_getint(field.bytes,field.size,1,&type)!=1 || type!=3)continue;
            while(diag_next(field.bytes,field.size,&b,&video)>0){
                uint32_t resolution=0,fps=0,width_margin=0,height_margin=0;
                if(video.number!=4 || video.wire!=2)continue;
                if(aa_getint(video.bytes,video.size,1,&resolution)!=1 || aa_getint(video.bytes,video.size,2,&fps)!=1)continue;
                aa_getint(video.bytes,video.size,3,&width_margin);aa_getint(video.bytes,video.size,4,&height_margin);
                note("diag advertised video service=%u resolution_enum=%u fps_enum=%u margins=%u,%u",id,resolution,fps,width_margin,height_margin);
                if(++printed==16)return;
            }
        }
    }
}
#endif
