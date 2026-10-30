/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.88: 128-point sound-shaping preview used by the main UI only.
 * No ISR calls, extra wavetable bank, saved format field or heap allocation.
 */
#pragma once
#include <stdint.h>
#define ETCH_WAVE_TOOL_PHASE_WARP 13u
#define ETCH_WAVE_TOOL_HALF_SYMM 14u
static inline int32_t etch_shape_bound(int32_t x,int32_t lo,int32_t hi){return x<lo?lo:x>hi?hi:x;}
static inline int16_t etch_shape_read(const int16_t s[128],uint32_t q8){
 uint32_t i=(q8>>8)&127u;uint32_t frac=q8&255u;
 return (int16_t)(((int32_t)s[i]*(int32_t)(256u-frac)+(int32_t)s[(i+1u)&127u]*(int32_t)frac)/256);
}
static inline void etch_wave_shape_preview(const int16_t source[128],int16_t result[128],uint8_t tool,int16_t amount){
 if(!source||!result)return;
 amount=(int16_t)etch_shape_bound(amount,-127,127);
 if(!amount || (tool!=ETCH_WAVE_TOOL_PHASE_WARP && tool!=ETCH_WAVE_TOOL_HALF_SYMM)){
  for(uint32_t i=0u;i<128u;i++)result[i]=source[i];return;
 }
 if(tool==ETCH_WAVE_TOOL_PHASE_WARP){
  /* Move the position of the original half-cycle boundary from 64 to
   * 16..112, keeping each half continuous and the wrap unchanged.
   * Resampling is done once per edit, NOT during audio playback. */
  uint32_t cut=(uint32_t)(64+(int32_t)amount*48/127);
  for(uint32_t i=0u;i<128u;i++){
   uint32_t phase=i<cut ? (i*64u*256u)/cut :
                         64u*256u+((i-cut)*64u*256u)/(128u-cut);
   /* Keep the last editable point before the periodic wrap: a sharp saw
    * edge belongs between sample 127 and sample 0, not inside sample 127. */
   if(i==127u)phase=127u*256u;
   result[i]=etch_shape_read(source,phase);
  }
 }else{
  /* Half-cycle polarity. +127 copies an inverted first half into the
   * second, suppressing even harmonics. -127 repeats the first half,
   * introducing prominent even harmonics / octave content. Preserve the
   * original half for intuitive editing and all interim amounts for morphs. */
  uint32_t strength=(uint32_t)(amount<0?-amount:amount);
  for(uint32_t i=0u;i<64u;i++)result[i]=source[i];
  for(uint32_t i=64u;i<128u;i++){
   int32_t target=amount>0?-(int32_t)source[i-64u]:(int32_t)source[i-64u];
   int32_t blended=(int32_t)source[i]+(int32_t)(((int64_t)(target-(int32_t)source[i])*strength)/127);
   result[i]=(int16_t)etch_shape_bound(blended,-32768,32767);
  }
 }
}
