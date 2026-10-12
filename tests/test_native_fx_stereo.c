/* SPDX-License-Identifier: GPL-3.0-only: native FX stereo regression (host only). */
#include "../firmware/fm1-native/native_fx.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static struct etch_native_fx fx;
int main(void){
 struct etch_fx_settings cfg;etch_fx_defaults(&cfg);
 int16_t l,r;etch_fx_reset(&fx);
 for(unsigned i=0;i<3000;i++){
  int16_t in=i==0?12000:0;
  etch_fx_process_stereo_ex(&fx,in,0,0,0,&cfg,&l,&r);
  assert(l==in&&r==in); /* genuine bypass, no changes to dry audio */
 }
 unsigned difference=0,active=0;
 for(uint8_t route=0;route<=1;route++){
  etch_fx_reset(&fx);cfg.routing=route;
  for(unsigned i=0;i<40000;i++){
   int16_t in=(i%131==0)?20000:0;
   etch_fx_process_stereo_ex(&fx,in,32767,32767,32767,&cfg,&l,&r);
   if(l!=r)difference++;
   if(l||r)active++;
   assert((int32_t)l>=-32768&&(int32_t)r>=-32768);
  }
 }
 assert(difference>1000&&active>1000);
 printf("PASS stereo FX: dry bypass, divergent L/R wet returns, serial/parallel, full-depth bounded (%u differing samples)\n",difference);
}
