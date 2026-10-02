/* Small standalone SHA-256 file checker for QNX 6.5. GPL-3.0-or-later.
 * Implements the standard SHA-256 compression function; no crypto SDK needed. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint32_t rotr(uint32_t x,unsigned n){return (x>>n)|(x<<(32-n));}
static const uint32_t k[64]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
static void block(uint32_t h[8],const unsigned char p[64]){
 uint32_t w[64],a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];unsigned i;
 for(i=0;i<16;i++)w[i]=((uint32_t)p[4*i]<<24)|((uint32_t)p[4*i+1]<<16)|((uint32_t)p[4*i+2]<<8)|p[4*i+3];
 for(i=16;i<64;i++){uint32_t x=w[i-15],y=w[i-2];w[i]=w[i-16]+(rotr(x,7)^rotr(x,18)^(x>>3))+w[i-7]+(rotr(y,17)^rotr(y,19)^(y>>10));}
 for(i=0;i<64;i++){
  uint32_t t=z+(rotr(e,6)^rotr(e,11)^rotr(e,25))+((e&f)^(~e&g))+k[i]+w[i];
  uint32_t u=(rotr(a,2)^rotr(a,13)^rotr(a,22))+((a&b)^(a&c)^(b&c));
  z=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u;
 }
 h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;
}
int main(int argc,char **argv){
 uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
 unsigned char p[64];size_t n,i;uint64_t bytes=0,bits;char hex[65];FILE *f;
 if(argc!=2 && argc!=3){fputs("Usage: altscreen-sha256 [expected-sha256] file\n",stderr);return 2;}
 if(argc==3 && (strlen(argv[1])!=64 || strspn(argv[1],"0123456789abcdef")!=64))return 2;
 f=fopen(argv[argc-1],"rb");if(!f){perror(argv[argc-1]);return 2;}
 while((n=fread(p,1,64,f))==64){bytes+=n;block(h,p);}
 if(ferror(f)){fclose(f);return 2;}fclose(f);bytes+=n;bits=bytes*8;
 p[n++]=0x80;if(n>56){memset(p+n,0,64-n);block(h,p);n=0;}memset(p+n,0,56-n);
 for(i=0;i<8;i++){p[63-i]=(unsigned char)(bits>>(8*i));}
 block(h,p);
 for(i=0;i<8;i++)sprintf(hex+8*i,"%08x",(unsigned)h[i]);
 if(argc==2){puts(hex);return 0;}
 if(strcmp(hex,argv[1])){fprintf(stderr,"SHA256 mismatch: %s\n",argv[2]);return 1;}return 0;
}
