/* ETCH V1.43 Wave Sequencing: step-local A/B waveform morph locks.
 * Uses the existing PJT30 lock lane, no change to stored structure.
 * Wave A and B are named, reusable saved waves assigned to this patch.
 * A signed encoder moves from A (0) through BLEND to B (32767).
 * Existing audio MORPH slew avoids abrupt discontinuities on held notes.
 */
#pragma once
#include <stdint.h>
#include "native_performance.h"
static inline uint8_t etch_wave_seq_enabled(const struct etch_seq_step *s){
 return (uint8_t)((s->lock_mask&(1u<<ETCH_LOCK_MORPH))!=0u);
}
static inline uint8_t etch_wave_seq_position(const struct etch_seq_step *s){
 uint32_t v=s->locks[ETCH_LOCK_MORPH];if(v>32767u)v=32767u;
 return (uint8_t)((v*100u+16383u)/32767u);
}
static inline uint8_t etch_wave_seq_mode(const struct etch_seq_step *s){
 uint16_t v=s->locks[ETCH_LOCK_MORPH];return v==0?0u:v>=32767u?2u:1u;
}
static inline void etch_wave_seq_set_position(struct etch_seq_step *s,uint8_t pct){
 if(pct>100u)pct=100u;
 s->locks[ETCH_LOCK_MORPH]=(uint16_t)((uint32_t)pct*32767u/100u);
 s->lock_mask|=(uint8_t)(1u<<ETCH_LOCK_MORPH);
}
static inline void etch_wave_seq_select(struct etch_seq_step *s,int32_t turns){
 if(!turns)return;
 int32_t m=etch_wave_seq_mode(s),n=m+turns;
 if(n<0)n=0;if(n>2)n=2;
 if(n==0)etch_wave_seq_set_position(s,0);
 else if(n==1)etch_wave_seq_set_position(s,50);
 else etch_wave_seq_set_position(s,100);
}
static inline void etch_wave_seq_toggle(struct etch_seq_step *s,int32_t steps){
 if(!steps)return;
 if(steps>0)s->lock_mask|=(uint8_t)(1u<<ETCH_LOCK_MORPH);
 else s->lock_mask&=(uint8_t)~(1u<<ETCH_LOCK_MORPH);
}
