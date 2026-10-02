#ifndef MU1438_AU_WIRE_H
#define MU1438_AU_WIRE_H
#include <stdint.h>
#include <string.h>
#define MU1438_AU_MAX (2u*1024u*1024u+4096u)
static inline void mu1438_au_header(unsigned char header[8],uint32_t length)
{
    memcpy(header,"AU11",4);
    header[4]=(unsigned char)(length>>24);header[5]=(unsigned char)(length>>16);
    header[6]=(unsigned char)(length>>8);header[7]=(unsigned char)length;
}
static inline uint32_t mu1438_au_length(const unsigned char header[8])
{
    uint32_t n;
    if(memcmp(header,"AU11",4))return 0;
    n=((uint32_t)header[4]<<24)|((uint32_t)header[5]<<16)|((uint32_t)header[6]<<8)|header[7];
    return n && n<=MU1438_AU_MAX?n:0;
}
#endif
