/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.15 / Evolution: time-varying TRANSFORMS of the actual A/B/C
 * carrier, not destructive edits of stored waveforms. Fixed-point DSP.
 */
#pragma once
#include <stdint.h>
struct etch_evolution_settings {uint8_t depth,rate,mode,sync;};
/* mode 0 fold, mode 1 sharpen, mode 2 staircase. sync 0 free, 1 tempo,
 * 2 one-shot; all inputs validated before persistent storage writes. */
static inline int etch_evolution_valid(const struct etch_evolution_settings *e){
 return e&&e->depth<=100u&&e->rate<=100u&&e->mode<3u&&e->sync<3u;
}
/* Triangle 0..32767, interpolated entirely per-sample, never a stepped LFO. */
static inline uint16_t etch_evolution_triangle(uint32_t phase){
 uint16_t v=(uint16_t)(phase>>16);
 return v<32768u?(uint16_t)v:(uint16_t)(65535u-v);
}
/* 0.06..~9.2 Hz at 44.1 kHz, with tempo 1/8..8 cycles/beat.
 * Recomputed once per render BLOCK, not once per voice/sample. */
static inline uint32_t etch_evolution_step(const struct etch_evolution_settings *e,uint8_t bpm){
 if(!etch_evolution_valid(e)||!e->depth)return 0;
 if(e->sync==1u){uint32_t cycles=1u+(uint32_t)e->rate*63u/100u;
  return (uint32_t)(((uint64_t)cycles*(bpm?bpm:120u)*4294967296ull)/(8u*60u*44100u));}
 return 5844u+(uint32_t)e->rate*888000u/100u;
}
static inline int16_t etch_evolution_shape(int16_t x,uint16_t amount,uint8_t mode){
 if(!amount)return x;
 int32_t altered=x;
 if(mode==0u){
  /* Triangle fold with a continuously changing drive (up to 4x). */
  int32_t hot=((int32_t)x*(32767+(int32_t)amount*3))/32767;
  while(hot>32767||hot<-32767){if(hot>32767)hot=65534-hot;else hot=-65534-hot;}
  altered=hot;
 }else if(mode==1u){
  /* SHARP: moderate symmetrical saturation and added odd harmonics. */
  int32_t hot=((int32_t)x*(32767+(int32_t)amount*3))/32767;
  altered=hot>32767?32767:hot<-32767?-32767:hot;
 }else if(mode==2u){
  /* STAIR: max 32 quantisation levels. Signal intentionally gritty. */
  int32_t quantum=1u+((uint32_t)amount*2047u)/32767u;
  altered=((int32_t)x/quantum)*quantum;
 }
 int32_t mix=(int32_t)x+(int32_t)(((int64_t)(altered-x)*amount)/32767);
 return (int16_t)(mix>32767?32767:mix<-32768?-32768:mix);
}
