/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.41: bounded, main-thread-only waveform sculpt transforms.
 * Source is never modified by preview. No persistent buffers or audio IRQ work.
 * The existing 128-point wave / PJT29 serializer stores committed results.
 */
#pragma once
#include <stdint.h>
#define ETCH_SCULPT_MODES 5u
static inline int32_t etch_sculpt_clamp(int32_t x,int32_t low,int32_t high){return x<low?low:x>high?high:x;}
static inline int32_t etch_sculpt_lerp(int32_t a,int32_t b,uint16_t wet){return a+(int32_t)(((int64_t)(b-a)*wet)/32767);}
static inline int32_t etch_sculpt_sample(const int16_t source[128],int32_t phase_q8){
 int32_t n=(phase_q8>>8)&127u;uint32_t frac=(uint32_t)phase_q8&255u;
 return ((int32_t)source[n]*(256-(int32_t)frac)+(int32_t)source[(n+1)&127u]*(int32_t)frac)/256;
}
/* Amount 0..32767, pivot 0..100 (50 = centre). Keep exact bypass for amount=0. */
static inline void etch_sculpt_preview(const int16_t source[128],int16_t output[128],
 uint8_t mode,uint16_t amount,uint8_t pivot){
 if(!source||!output)return;
 if(amount>32767u)amount=32767u;if(pivot>100u)pivot=100u;
 if(!amount||mode>=ETCH_SCULPT_MODES){for(uint32_t i=0;i<128;i++)output[i]=source[i];return;}
 const int32_t anchor=16+(int32_t)pivot*96/100;
 for(int32_t i=0;i<128;i++){
  const int32_t dry=source[i];int32_t wet=dry;
  if(mode==0){ /* fold: triangle reflection, bounded below 6 reflections */
   int32_t amp=32767+(int32_t)(amount*3u);int32_t x=(int32_t)(((int64_t)dry*amp)/32767);
   for(uint8_t k=0;k<8u&&(x>32767||x< -32768);k++)x=x>32767?65534-x:-65536-x;
   wet=etch_sculpt_clamp(x,-32768,32767);
  }else if(mode==1){ /* bend: time warp with movable two-segment pivot */
   int32_t target=i<64 ? (i*anchor*256)/64 : (anchor*256)+((i-64)*(127-anchor)*256)/63;
   wet=etch_sculpt_sample(source,etch_sculpt_clamp(target,0,127*256));
  }else if(mode==2){ /* asymmetry: opposite gain on positive and negative lobes */
   int32_t bias=(int32_t)pivot-50;
   int32_t gain=32767+(dry>=0?bias:-bias)*512;
   wet=etch_sculpt_clamp((int32_t)(((int64_t)dry*gain)/32767),-32768,32767);
  }else if(mode==3){ /* phase distortion: endpoint-safe quadratic warping */
   int32_t target=(i*127*256)/127;
   if(i<=anchor){target=(i*i*256)/anchor;}
   else {int32_t t=127-i;int32_t div=127-anchor;
    target=(127-(t*t)/div)*256;
   }
   wet=etch_sculpt_sample(source,etch_sculpt_clamp(target,0,127*256));
  }else if(mode==4){ /* harmonic tilt: diffuse or enhance high-frequency detail */
   int32_t near=((int32_t)source[(i+127)&127]+source[(i+1)&127])/2;
   int32_t high=dry-near;
   wet=etch_sculpt_clamp(dry+(int32_t)(((int64_t)high*((int32_t)pivot-50)*3)/50),-32768,32767);
  }
  output[i]=(int16_t)etch_sculpt_clamp(etch_sculpt_lerp(dry,wet,amount),-32768,32767);
 }
}
