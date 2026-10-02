#include "rgd_wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void put16(uint8_t *p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static unsigned get16(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
static size_t start(uint8_t *p,unsigned msg){p[0]=p[1]=0x40;put16(p+4,msg);return 6;}
static void end(uint8_t *p,size_t n){put16(p+2,(unsigned)n);}
static size_t scalar(uint8_t *p,size_t at,unsigned id,uint64_t value,unsigned width){
    unsigned i;put16(p+at,width+4);put16(p+at+2,id);
    for(i=0;i<width;i++)p[at+4+i]=(uint8_t)(value>>(8*(width-1-i)));
    return at+4+width;
}
static size_t text(uint8_t *p,size_t at,unsigned id,const char *value){
    size_t n=strlen(value)+1;put16(p+at,(unsigned)n+4);put16(p+at+2,id);memcpy(p+at+4,value,n);return at+4+n;
}
static int list_has(const uint8_t *p,size_t n,unsigned key,unsigned id){
    size_t a,j;for(a=6;a<n;a+=get16(p+a))if(get16(p+a+2)==key){for(j=a+4;j<a+get16(p+a);j+=2)if(get16(p+j)==id)return 1;}return 0;
}
int main(void){
    uint8_t p[4096],copy[4096],out[4096],before[4096];struct rgd_event e,unchanged;
    size_t n,k,a;unsigned i;uint32_t random=0x31415926;
    memset(&e,0x5a,sizeof(e));unchanged=e;
    n=start(p,0x5201);n=scalar(p,n,1,1,1);n=scalar(p,n,5,0x0102030405060708ULL,8);
    n=scalar(p,n,10,250,4);n=text(p,n,3,"PRIVATE ROAD");end(p,n);memcpy(copy,p,n);
    assert(rgd_parse(p,n,&e)==1);assert(e.value[1]==1 && e.value[10]==250 && e.value[5]==0x0102030405060708ULL);
    assert(e.length[3]==13 && !(e.numeric&(1u<<3)));assert(!memcmp(p,copy,n));
    for(k=0;k<n;k++){e=unchanged;assert(rgd_parse(p,k,&e)==-1);assert(!memcmp(&e,&unchanged,sizeof(e)));}
    n=scalar(p,n,1,2,1);end(p,n);e=unchanged;assert(rgd_parse(p,n,&e)==-1);assert(!memcmp(&e,&unchanged,sizeof(e)));
    n=start(p,0x5202);n=scalar(p,n,1,700,2);n=scalar(p,n,11,0xffa6,2);end(p,n);
    assert(rgd_parse(p,n,&e)==1 && e.value[1]==700 && (int16_t)e.value[11]==-90);
    n=start(p,0x5201);n=scalar(p,n,13,1,1);end(p,n);assert(rgd_parse(p,n,&e)==-1);
    n=start(p,0x5201);n=scalar(p,n,99,23,1);end(p,n);assert(rgd_parse(p,n,&e)==1 && e.unknown_fields==1);
    n=start(p,0x1d01);n=text(p,n,0,"test accessory");n=scalar(p,n,6,0x5000,2);n=scalar(p,n,7,0x5001,2);end(p,n);memcpy(copy,p,n);
    memset(out,0xa5,sizeof(out));k=rgd_identify(p,n,16,out,sizeof(out));assert(k>n && get16(out+2)==k && !memcmp(p,copy,n));
    assert(!memcmp(p+6,out+6,get16(p+6))); /* preserve accessory identity */
    assert(list_has(out,k,6,0x5000) && list_has(out,k,6,0x5200) && list_has(out,k,6,0x5203));
    assert(list_has(out,k,7,0x5201) && list_has(out,k,7,0x5202) && list_has(out,k,7,0x5204));
    assert(!list_has(out,k,7,0x5200) && !list_has(out,k,7,0x5203));
    for(a=6;a<k && get16(out+a+2)!=30;a+=get16(out+a)){}
    assert(a<k && get16(out+a+8)==16);
    memset(before,0xa5,sizeof(before));memcpy(out,before,sizeof(out));
    assert(rgd_identify(p,n,16,out,6)==0 && !memcmp(out,before,sizeof(out)));
    assert(rgd_identify(p,n,16,p,sizeof(p))==0 && !memcmp(p,copy,n));
    k=rgd_identify(p,n,16,out,sizeof(out));assert(k);memcpy(copy,out,k);
    assert(rgd_identify(copy,k,16,out,sizeof(out))==0); /* no double advertisement */
    for(i=0;i<100000;i++){
        size_t j;random=random*1664525u+1013904223u;n=random%512;
        for(j=0;j<n;j++){random=random*1664525u+1013904223u;p[j]=(uint8_t)(random>>24);}
        if(n>=6 && (i&1)){p[0]=p[1]=0x40;put16(p+2,(unsigned)n);put16(p+4,0x5200+i%5);}
        e=unchanged;if(rgd_parse(p,n,&e)!=1)assert(!memcmp(&e,&unchanged,sizeof(e)));
        if(n>=6)put16(p+4,0x1d01);
        memcpy(out,before,sizeof(out));k=rgd_identify(p,n,16,out,sizeof(out));
        if(!k)assert(!memcmp(out,before,sizeof(out)));
    }
    puts("PASS: numeric metadata, text redaction, truncation/duplicates/lengths, unknown fields, transactional negotiation, input preservation, 100000 generated cases");
    return 0;
}
