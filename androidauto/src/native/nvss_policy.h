#ifndef AA_NVSS_POLICY_H
#define AA_NVSS_POLICY_H
#include <stdint.h>
/* Owned MU1438 gal: 0x1e7008 passes 35ms; 0x1e70d0 distinguishes timeout
 * (retry) from fatal failure; gal.json sets retryCount=5. Disabling timestamp
 * scheduling uses UINT64_MAX at 0x1e7268, not a zero presentation time. */
#define AA_NVSS_WAIT_MS 35
#define AA_NVSS_MAX_RETRIES 5
#define AA_NVSS_NO_TIMESTAMP UINT64_MAX
static int aa_nvss_retry(int status,unsigned retries)
{
    return (uint32_t)status==0x80000006u && retries<AA_NVSS_MAX_RETRIES;
}
#endif
