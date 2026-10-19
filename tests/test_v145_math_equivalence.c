/* SPDX-License-Identifier: GPL-3.0-only
 * V1.45 exact signed Q15 filter multiply against V1.44 int64 reference.
 * Requires audio_core.c for the identical inline helper used in firmware. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "../firmware/fm1-native/audio_core.c"
int main(void){
 static const int32_t edges[]={-262144,-262143,-65536,-32769,-32768,-32767,-1,0,1,32767,32768,65536,262143,262144};
 static const int32_t coefs[]={0,1,512,8192,16386,32767,32768,65535,65536};
 unsigned long tested=0;
 for(unsigned i=0;i<sizeof(edges)/sizeof(edges[0]);i++)for(unsigned j=0;j<sizeof(coefs)/sizeof(coefs[0]);j++){
  int32_t x=edges[i],f=coefs[j];
  assert(filter_mul_q15(x,f)==(int32_t)(((int64_t)x*f)/32768));tested++;
 }
 uint32_t seed=0x05abcdefu;
 for(unsigned i=0;i<1000000u;i++){
  seed=1664525u*seed+1013904223u;
  int32_t x=(int32_t)(seed%524289u)-262144;
  seed=1664525u*seed+1013904223u;
  int32_t f=(int32_t)(seed%65537u);
  assert(filter_mul_q15(x,f)==(int32_t)(((int64_t)x*f)/32768));tested++;
 }
 printf("PASS Q15 filter 32-bit math == V1.44 64-bit math for %lu signed cases\n",tested);
}
