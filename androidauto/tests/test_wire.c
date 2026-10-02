#include "aa_wire.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    unsigned char data[128],small[4]={0xaa,0xaa,0xaa,0xaa};uint32_t v=0;
    const unsigned char truncated[]={8,128};
    const unsigned char overflow[]={8,255,255,255,255,255,255,255,255,255,2};
    const unsigned char mixed[]={18,2,1,2,8,150,1};
    struct aa_pb b={data,0,sizeof(data),0};size_t n=aa_cluster_services(data,sizeof(data));
    assert(n==40);assert(data[0]==10 && data[1]==30 && data[2]==8 && data[3]==19);
    assert(data[32]==10 && data[33]==6 && data[34]==8 && data[35]==20);
    assert(aa_cluster_services(small,2)==0);assert(small[2]==0xaa&&small[3]==0xaa);
    aa_int(&b,1,UINT32_MAX);assert(!b.bad && aa_getint(data,b.used,1,&v)==1 && v==UINT32_MAX);
    assert(aa_getint(truncated,sizeof(truncated),1,&v)==-1);
    assert(aa_getint(overflow,sizeof(overflow),1,&v)==-1);
    assert(aa_getint(mixed,sizeof(mixed),1,&v)==1 && v==150);
    assert(aa_getint(mixed,sizeof(mixed),9,&v)==0);
    puts("PASS: discovery descriptors, bounded writes, session varints, truncated and oversized inputs");
    return 0;
}
