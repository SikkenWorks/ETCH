/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.76 Organic Engine: click-free, slowly interpolated random drift.
 * Four 0..100 controls. Zeroed project settings are a true, bit-exact bypass.
 */
#pragma once
#include <stdint.h>
struct etch_organic_settings {
 uint8_t drift, rate, phase, variance;
};
static inline uint8_t etch_organic_valid(const struct etch_organic_settings *o){
 return o && o->drift<=100u && o->rate<=100u && o->phase<=100u && o->variance<=100u;
}
static inline uint32_t etch_organic_hash(uint32_t x){
 x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x;
}
/* +/-25 cents maximum = ~950 Q16 pitch units at full depth.
 * STATIC variance adds up to +/-10 cents. Zero still bypasses exactly. */
static inline int32_t etch_organic_target(uint32_t seed,uint8_t depth){
 uint32_t span=(uint32_t)depth*950u/100u;
 return (int32_t)((uint64_t)etch_organic_hash(seed)*(2u*span+1u)>>32)-(int32_t)span;
}
static inline int32_t etch_organic_static(uint32_t seed,uint8_t depth){
 uint32_t span=(uint32_t)depth*380u/100u;
 return (int32_t)((uint64_t)etch_organic_hash(seed)*(2u*span+1u)>>32)-(int32_t)span;
}
static inline uint32_t etch_organic_initial_phase(uint32_t seed,uint8_t depth){
 return (uint32_t)(((uint64_t)etch_organic_hash(seed)*depth)/100u);
}
/* Random target changes are separated by 2.5..9 seconds (44.1 kHz).
 * The former 0.09..0.91-second target cadence, combined with a 12ms
 * one-pole response, produced the audible staircase reported on hardware.
 * At 48kHz the actual durations are about 8% longer, still smooth.
 * Calculate at control rate; NEVER add extra work to the PCM inner loop. */
static inline uint32_t etch_organic_interval(uint8_t rate){
 return 396900u-(uint32_t)rate*2866u;
}
/* Total 32-sample control ticks fits uint16_t even at the slowest rate.
 * A 32-bit per-voice `time` packs duration:elapsed and does not grow SRAM. */
static inline uint16_t etch_organic_duration_ticks(uint8_t rate){
 return (uint16_t)((etch_organic_interval(rate)+31u)/32u);
}
/* Zero-velocity quadratic ease-in/out between randomized +/-25-cent targets.
 * Q15 evaluation: all intermediates fit in unsigned/signed 32 bits.
 * Keeping start and end in the existing voice state gives continuous pitch,
 * and near-zero slope at both ends, without the old per-tick staircase. */
static inline int32_t etch_organic_smooth_pitch(int32_t start,int32_t end,
                                                 uint16_t elapsed,uint16_t duration){
 if(!duration||elapsed>=duration)return end;
 uint32_t x=((uint32_t)elapsed*32768u)/duration;
 /* Exactly monotonic even after integer quantization, with zero slope
  * at both endpoints. No multiply is wider than 32 bits. */
 uint32_t eased;
 if(x<=16384u)eased=(x*x)>>14;
 else{uint32_t inverse=32768u-x;eased=32768u-((inverse*inverse)>>14);}
 return start+(int32_t)(((end-start)*(int32_t)eased)/32768);
}
static inline uint32_t etch_organic_pitch_inc(uint32_t inc,int32_t delta_q16){
 int64_t result=(int64_t)inc+(((int64_t)inc*delta_q16)>>16);
 return (uint32_t)(result<1?1:result>0xffffffffLL?0xffffffffLL:result);
}
