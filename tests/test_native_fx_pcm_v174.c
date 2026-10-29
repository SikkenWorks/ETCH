/* SPDX-License-Identifier: GPL-3.0-only
 * Deterministic FX waveform regression: compare native PCM checksum against
 * pre-optimisation V1.73 in a separate, known-good host build.
 */
#include "../firmware/fm1-native/native_fx.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static struct etch_native_fx fx;
static uint32_t rng=0x1acfb00du;
static uint32_t random32(void){rng=rng*1664525u+1013904223u;return rng;}
static uint64_t hashbyte(uint64_t h,uint8_t b){return (h^b)*1099511628211ull;}
static uint64_t hash16(uint64_t h,int16_t v){uint16_t u=(uint16_t)v;return hashbyte(hashbyte(h,(uint8_t)u),(uint8_t)(u>>8));}
int main(void){
 struct etch_fx_settings cfg; etch_fx_defaults(&cfg); etch_fx_reset(&fx);
 uint64_t sums[4];
 for(unsigned scene=0;scene<4u;scene++){
  uint64_t h=14695981039346656037ull;
  for(unsigned i=0;i<28000u;i++){
   if(i%319u==0u){
    uint32_t r=random32();
    cfg.routing=(uint8_t)(r>>31);
    cfg.reverb_size=(uint16_t)(r&32767u);
    cfg.reverb_decay=(uint16_t)((r>>12)&32767u);
    cfg.delay_feedback=(uint16_t)((r>>7)&32767u);
    cfg.ring_rate=(uint16_t)((r>>5)&32767u);
    cfg.ring_mix=scene==0?0:(uint16_t)((r>>4)&12000u);
    cfg.eq_low=scene==0?16384u:(uint16_t)(4096u+(r&16383u));
    cfg.eq_mid=scene==0?16384u:(uint16_t)(8192u+((r>>8)&16383u));
    cfg.eq_high=scene==0?16384u:(uint16_t)(8192u+((r>>12)&16383u));
    cfg.eq_hpf=scene>=2?(uint16_t)(r&16383u):0;
   }
   int16_t in=(int16_t)(random32()>>16);
   uint16_t chorus=scene==0?0u:(uint16_t)(12000u+((i*19u)&16383u));
   uint16_t delay=scene==0?0u:(uint16_t)(8000u+((i*7u)&8191u));
   uint16_t reverb=scene==0?0u:(uint16_t)(5000u+((i*5u)&16383u));
   if(scene==3 && i>9000u && i<19000u){
    chorus=delay=reverb=0;cfg.ring_mix=cfg.eq_hpf=0;
    cfg.eq_low=cfg.eq_mid=cfg.eq_high=16384;
   }
   int16_t l=0,r=0;
   etch_fx_process_stereo_ex(&fx,in,chorus,delay,reverb,&cfg,&l,&r);
   h=hash16(hash16(h,l),r);
  }
  sums[scene]=h;
 }
 const uint64_t golden[4]={0x5af1e7ebab11c715ull,0xe0635d464b8afaf7ull,
    0x6583fb2ba6a44505ull,0xf2049c34dc44e9e7ull};
 for(unsigned i=0;i<4u;i++){
   printf("scene%u=%016llx\n",i,(unsigned long long)sums[i]);
   if(sums[i]!=golden[i]){fprintf(stderr,"V1.73 PCM parity failed in scene %u\n",i);return 1;}
 }
 puts("PASS: four V1.73 PCM reference renders unchanged");
 return 0;
}
