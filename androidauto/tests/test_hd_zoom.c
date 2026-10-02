#include "aa_wire.h"
#include <assert.h>
#include <stdio.h>
static void dump(const unsigned char *p,size_t n)
{
    size_t i;assert(n);for(i=0;i<n;i++)printf("%02x",p[i]);puts("");
}
int main(void)
{
    unsigned char data[256];size_t n;
    n=aa_cluster_services(data,sizeof(data));assert(n<=128);dump(data,n);
    n=aa_zoom_report(data,sizeof(data),123456,-1,0,1);dump(data,n);
    n=aa_zoom_report(data,sizeof(data),123457,-1,0,0);dump(data,n);
    n=aa_zoom_report(data,sizeof(data),123458,1,0,1);dump(data,n);
    n=aa_zoom_report(data,sizeof(data),123459,-2,1,1);dump(data,n);
    assert(!aa_zoom_report(data,2,123,-2,1,1));
    return 0;
}
