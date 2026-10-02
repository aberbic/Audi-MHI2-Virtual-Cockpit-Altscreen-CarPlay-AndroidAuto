#include "nvss_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    unsigned i;
    assert(AA_NVSS_WAIT_MS==35 && AA_NVSS_NO_TIMESTAMP==UINT64_MAX);
    for(i=0;i<5;i++)assert(aa_nvss_retry((int)0x80000006u,i));
    assert(!aa_nvss_retry((int)0x80000006u,5));
    assert(!aa_nvss_retry((int)0x80000003u,0));
    assert(!aa_nvss_retry(0,0));
    puts("PASS: stock 35ms wait, absent-timestamp sentinel, bounded timeout retry, no retry for fatal/success");
    return 0;
}
