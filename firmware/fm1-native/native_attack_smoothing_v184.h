/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.84: zero-slope, 256-frame voice-on ramp to suppress note transients.
 * Executed only while a voice is entering. No persistent memory or LUT.
 */
#pragma once
#include <stdint.h>
#define ETCH_ATTACK_SAMPLES 256u
static inline uint16_t etch_attack_gain_q8(uint16_t remaining){
 uint32_t t=(remaining>=ETCH_ATTACK_SAMPLES)?0u:ETCH_ATTACK_SAMPLES-(uint32_t)remaining;
 /* smoothstep(t/256) in Q8, 32-bit bound <= 50,331,648 */
 return (uint16_t)((t*t*(768u-2u*t)+32768u)>>16);
}
static inline int32_t etch_attack_apply(int32_t sample,uint16_t remaining){
 uint32_t gain=etch_attack_gain_q8(remaining);
 /* Bound intermediates using a quotient/remainder decomposition. Do not
  * add a software 64-bit multiply in the per-voice audio path. */
 int32_t coarse=(sample/256)*(int32_t)gain;
 int32_t fine=((sample%256)*(int32_t)gain)/256;
 return coarse+fine;
}

/* V1.85: rounded bank/voice transitions, no extra DSP state or LUT.
 * 0..total remaining samples -> a 0..256 old-signal contribution.
 * The two endpoints have zero slope, unlike linear fade boundaries. */
static inline uint16_t etch_transition_old_mix_q8(uint32_t remaining,uint32_t total){
 if(!total)return 0u;
 if(remaining>=total)return 256u;
 uint32_t scaled=(remaining*256u+total/2u)/total;
 return etch_attack_gain_q8((uint16_t)(256u-scaled));
}
