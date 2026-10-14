/* SPDX-License-Identifier: GPL-3.0-only
 * Internal implementation equivalence: mip level and boundary fade cached
 * every control tick must give exactly the same PCM wavetable read. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/audio_core.c"
int main(void){
 static const uint32_t bounds[]={30198988u,60397977u,120795955u,241591910u,966367641u};
 uint32_t seed=0x501abcdu;
 for(uint32_t wave=0;wave<3;wave++){
  for(uint32_t j=0;j<400;j++){
   uint32_t inc;
   if(j<5)inc=bounds[j];
   else if(j<10)inc=bounds[j-5]-1u;
   else if(j<15)inc=bounds[j-10]+1u;
   else if(j==15)inc=0;
   else if(j==16)inc=0xffffffffu;
   else {seed=seed*1664525u+1013904223u;inc=seed;}
   struct etch_mip_lookup prof=mip_prepare(inc);
   for(uint32_t k=0;k<20;k++){
    seed=seed*1664525u+1013904223u;
    assert(readwave(0,wave,seed,inc)==readwave_prepared(0,wave,seed,&prof));
   }
  }
 }
 puts("PASS cached mip output is bit-identical for 24,000 level/threshold/phase lookups");
}
