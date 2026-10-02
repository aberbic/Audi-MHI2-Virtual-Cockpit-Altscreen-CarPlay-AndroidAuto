#include "rgd_monitor.h"
#include "rgd_packet.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static unsigned count,last;
static void received(void *x,const uint8_t *p,size_t n){(void)x;assert(n==6);last=((unsigned)p[4]<<8)|p[5];count++;}
static void put16(uint8_t *p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void check(uint8_t *p,size_t n){unsigned sum=0;size_t i;for(i=0;i<n-1;i++)sum+=p[i];p[n-1]=(uint8_t)-sum;}
static size_t packet(uint8_t *p,unsigned flags,unsigned seq,unsigned session,const uint8_t *data,size_t n){
    size_t size=n?10+n:9;p[0]=0xff;p[1]=0x5a;put16(p+2,(unsigned)size);p[4]=(uint8_t)flags;p[5]=(uint8_t)seq;p[6]=0;p[7]=(uint8_t)session;check(p,9);
    if(n){memcpy(p+9,data,n);check(p+9,n+1);}return size;
}
static const uint8_t syn[]={1,8,4,0,0,100,0,50,5,3,7,0,1,8,2,1};
static const uint8_t message[]={0x40,0x40,0,6,0x52,1};
int main(void){
    static struct rgd_monitor s;uint8_t p[512],storage[1024],original[1024];size_t n,i;
    struct rgd_native_packet native;
    rgd_monitor_init(&s,received,NULL);
    n=packet(p,0x80,1,0,syn,sizeof(syn));for(i=0;i<n;i++)rgd_monitor_feed(&s,p+i,1);
    assert(s.control_known && s.control_id==7);
    n=packet(p,0x40,2,7,message,3);rgd_monitor_feed(&s,p,n);
    assert(!count && s.message_used==3);
    n=packet(p,0x40,3,7,message+3,3);for(i=0;i<n;i++)rgd_monitor_feed(&s,p+i,1);
    assert(count==1 && last==0x5201);
    rgd_monitor_feed(&s,p,n);assert(count==1 && s.duplicates==1);
    n=packet(p,0x40,4,8,message,6);rgd_monitor_feed(&s,p,n);assert(count==1);
    n=packet(p,0x40,5,7,message,6);p[n-1]^=1;rgd_monitor_feed(&s,p,n);assert(count==1 && s.bad_packets==1);
    n=packet(p,0x40,6,7,message,6);rgd_monitor_feed(&s,p,n);assert(count==2 && s.gaps);
    n=packet(p,0x10,7,7,NULL,0);rgd_monitor_feed(&s,p,n);assert(!s.control_known && !s.message_used);
    n=packet(p,0x40,8,7,message,6);rgd_monitor_feed(&s,p,n);assert(count==2);
    memset(storage,0x5a,sizeof(storage));memset(&native,0,sizeof(native));
    native.storage=storage;native.control=storage+9;native.capacity=sizeof(storage);native.payload_length=native.total=12;
    put16(storage+15,6);put16(storage+17,6);put16(storage+19,0x5000);
    put16(storage+21,6);put16(storage+23,7);put16(storage+25,0x5001);
    memcpy(original,storage,sizeof(storage));
    assert(rgd_packet_identify(&native,16)==1 && native.payload_length>12);
    assert(!memcmp(storage,original,15));
    assert(native.total==native.payload_length);
    memcpy(original,storage,sizeof(storage));assert(rgd_packet_identify(&native,16)==0);assert(!memcmp(storage,original,sizeof(storage)));
    native.capacity=22;assert(rgd_packet_identify(&native,16)==0);assert(!memcmp(storage,original,sizeof(storage)));
    puts("PASS: negotiated session, bytewise reads, fragmented messages, duplicates, other channels, checksums, gaps, resets, bounded native packet edits");return 0;
}
