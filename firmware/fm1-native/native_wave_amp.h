/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.84 WAVE AMP: phase-locked, audio-rate gain from the existing
 * drawn Wave A, Wave B, or their morph. NO extra oscillator or wavetable.
 * PJT32: reuse two previously reserved bits of spectral_wave and six
 * reserved high bits of velocity_curve without changing record length.
 */
#pragma once
#include <stdint.h>
#define ETCH_WAMP_OFF 0u
#define ETCH_WAMP_A 1u
#define ETCH_WAMP_B 2u
#define ETCH_WAMP_MORPH 3u
static inline uint8_t etch_wamp_mode(uint8_t packed_wave){return (uint8_t)((packed_wave>>2)&3u);}
static inline uint8_t etch_wamp_spectral(uint8_t packed_wave){return packed_wave&3u;}
static inline uint8_t etch_wamp_velocity(uint8_t packed_velocity){return packed_velocity&3u;}
static inline uint8_t etch_wamp_depth6(uint8_t packed_velocity){return packed_velocity>>2;}
static inline uint8_t etch_wamp_pack_wave(uint8_t spectral,uint8_t mode){return (uint8_t)((spectral&3u)|((mode&3u)<<2));}
static inline uint8_t etch_wamp_pack_velocity(uint8_t curve,uint8_t depth6){return (uint8_t)((curve&3u)|((depth6&63u)<<2));}
static inline uint8_t etch_wamp_percent(uint8_t depth6){return (uint8_t)(((uint32_t)depth6*100u+31u)/63u);}
static inline uint8_t etch_wamp_from_percent(uint8_t percent){return (uint8_t)(((uint32_t)(percent>100u?100u:percent)*63u+50u)/100u);}
static inline uint16_t etch_wamp_q15(uint8_t depth6){return (uint16_t)(((uint32_t)depth6*32768u+31u)/63u);}
/* Map signed oscillator [-32768,32767] into Q15 volume [0,32768]. */
static inline uint16_t etch_wamp_unipolar(int32_t wave){
 if(wave < -32768)wave=-32768;
 if(wave>32767)wave=32767;
 return (uint16_t)((wave+32769)>>1);
}
/* Bounded 32-bit Q15 multiplies; zero mode is a true no-op. */
static inline int32_t etch_wamp_apply(int32_t sample,int32_t wave,uint16_t depth){
 if(!depth)return sample;
 int32_t level=etch_wamp_unipolar(wave);
 uint32_t attenuation=(uint32_t)((32768-level)*(uint32_t)depth)>>15;
 int32_t gain=32768-(int32_t)attenuation;
 return (int32_t)((sample*gain)>>15);
}
static inline int32_t etch_wamp_source(uint8_t mode,int32_t a,int32_t b,uint16_t morph){
 if(mode==ETCH_WAMP_A)return a;
 if(mode==ETCH_WAMP_B)return b;
 return a+(int32_t)(((b-a)*(int32_t)morph)>>15);
}
