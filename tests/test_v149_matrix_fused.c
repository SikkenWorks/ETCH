/* SPDX-License-Identifier: GPL-3.0-only
 * Bit-for-bit comparison of the V1.48.5 matrix and V1.49 fused path.
 * Run with ASan and UBSan on the host, with deterministic pseudorandom data.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../firmware/fm1-native/native_mod_matrix.h"
static uint32_t seed=0xc0ffee17u;
static uint32_t rnd(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void){
 const uint32_t cycles=500000u;
 for(uint32_t t=0;t<cycles;t++){
  struct etch_mod_matrix m;
  for(uint8_t r=0;r<ETCH_MATRIX_ROUTES;r++){
   m.route[r].source=(uint8_t)(rnd()%7u);
   m.route[r].target=(uint8_t)(rnd()%4u);
   m.route[r].amount=(uint8_t)(rnd()%201u);
  }
  uint16_t old[4],now[4];
  for(int j=0;j<4;j++)now[j]=old[j]=(uint16_t)(rnd()%32768u);
  uint8_t vel=(uint8_t)(rnd()%128u),pr=(uint8_t)(rnd()%128u),note=(uint8_t)(rnd()%128u);
  uint16_t env=(uint16_t)(rnd()%65536u),living=(uint16_t)(rnd()%65536u);
  int16_t wc=(int16_t)rnd();
  int has_space=(rnd()&1u)!=0;
  for(uint8_t d=0;d<4u;d++)if(d!=3u||has_space)
   old[d]=etch_matrix_apply_active_ex(&m,d,old[d],vel,env,wc,living,pr,note);
  etch_matrix_apply_all_active_ex(&m,&now[0],&now[1],&now[2],has_space?&now[3]:0,vel,env,wc,living,pr,note);
  for(int j=0;j<4;j++)if(old[j]!=now[j]){
   fprintf(stderr,"Mismatch case %u, destination %d: expected %u, got %u\n",t,j,old[j],now[j]);return 1;
  }
 }
 printf("PASS %u random matrix cases, 4 destinations, optional SPACE\n",cycles);
 return 0;
}
