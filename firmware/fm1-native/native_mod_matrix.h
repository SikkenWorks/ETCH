/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.42 MOD MATRIX 2.0: 4 realtime-safe routes in 12 bytes.
 * Eight lanes are NOT claimed: per-preset RAM is saturated on FM1.
 * PJT30 adds signed-depth encoding and AFTERTOUCH/KEYTRACK sources.
 */
#pragma once
#include <stdint.h>
#define ETCH_MATRIX_ROUTES 4u
#define ETCH_MATRIX_MAX_SOURCE 6u
/* SOURCE: 0 OFF,1 VELOCITY,2 ENVELOPE,3 WAVE C,4 LIVING,
 *         5 CHANNEL PRESSURE,6 KEYTRACK.
 * TARGET: 0 MORPH,1 WAVE C DEPTH,2 FILTER CUTOFF,3 SPACE.
 * Amount <=100 is positive. 101..200 encodes -1..-100.
 * The old 0..100 representation stays byte-for-byte compatible. */
struct etch_mod_route {uint8_t source,target,amount;};
struct etch_mod_matrix {struct etch_mod_route route[ETCH_MATRIX_ROUTES];};
_Static_assert(sizeof(struct etch_mod_matrix)==12u,"matrix must remain exactly 12 bytes");
static inline int32_t etch_matrix_signed(uint8_t raw){return raw<=100u?(int32_t)raw:100-(int32_t)raw;}
static inline uint8_t etch_matrix_encode_signed(int32_t depth){
 if(depth>100)depth=100;
 if(depth< -100)depth=-100;
 return (uint8_t)(depth<0?100-depth:depth);
}
static inline uint8_t etch_matrix_valid(const struct etch_mod_matrix *m){
 if(!m)return 0;
 for(uint32_t i=0;i<ETCH_MATRIX_ROUTES;i++)if(m->route[i].source>ETCH_MATRIX_MAX_SOURCE||
     m->route[i].target>3u||m->route[i].amount>200u)return 0;
 return 1;
}
/* PJT24..29 never encoded signed routes or sources beyond LIVING. */
static inline uint8_t etch_matrix_valid_legacy(const struct etch_mod_matrix *m){
 if(!etch_matrix_valid(m))return 0;
 for(uint32_t i=0;i<ETCH_MATRIX_ROUTES;i++)if(m->route[i].source>4u||m->route[i].amount>100u)return 0;
 return 1;
}
static inline uint8_t etch_matrix_active(const struct etch_mod_matrix *m){
 if(!etch_matrix_valid(m))return 0;
 for(uint32_t i=0;i<ETCH_MATRIX_ROUTES;i++)if(m->route[i].source&&m->route[i].amount)return 1;
 return 0;
}
/* Normalized positive values for envelope, velocity, aftertouch, keytracking;
 * Wave C bipolar. A route contributes at most +/-25% of the 15-bit range.
 * Saturation occurs AFTER summing all four routes to avoid order dependence.
 * The hot path uses bounded int32; source*depth <= 3,276,700. */
/* Only call with a previously validated, active and immutable matrix.
 * The audio renderer resolves this once per DMA block; this intentionally
 * skips the repeated 4-route validation/scan on EVERY sample/voice/target.
 * Other users retain the validating public wrapper below. */
static inline uint16_t etch_matrix_apply_active_ex(const struct etch_mod_matrix *m,
 uint8_t target,uint16_t base,uint8_t velocity,uint16_t envelope,
 int16_t wave_c,uint16_t living,uint8_t pressure,uint8_t note){
 int32_t result=(int32_t)base;
 for(uint32_t i=0;i<ETCH_MATRIX_ROUTES;i++){
  const struct etch_mod_route *r=&m->route[i];
  if(!r->source||!r->amount||r->target!=target)continue;
  int32_t src=0;
  switch(r->source){
   case 1:src=(int32_t)velocity*32767/127;break;
   case 2:src=envelope>32767u?32767:(int32_t)envelope;break;
   case 3:src=wave_c;break;
   case 4:src=living>32767u?32767:(int32_t)living;break;
   case 5:src=(int32_t)pressure*32767/127;break;
   case 6:src=(int32_t)note*32767/127;break;
   default:break;
  }
  result+=(src*etch_matrix_signed(r->amount))/400;
 }
 return (uint16_t)(result<0?0:result>32767?32767:result);
}
/* V1.49: evaluate the four immutable routes ONE time for all targets.
 * The historical four calls each traversed all four routes, even if the
 * route belonged to a different target. Accumulate independently by target,
 * preserving the original per-route integer division and final saturation.
 * Null SPACE pointer skips that global destination for GROUPS and subsequent
 * voices, preserving the original first-main-voice policy.
 * Audio callback only: caller already verified an active immutable matrix.
 * No allocations, floating point or additional persistent memory.
 */
static inline void etch_matrix_apply_all_active_ex(const struct etch_mod_matrix *m,
 uint16_t *morph,uint16_t *depth,uint16_t *cutoff,uint16_t *space,
 uint8_t velocity,uint16_t envelope,int16_t wave_c,uint16_t living,
 uint8_t pressure,uint8_t note){
 int32_t values[4]={(int32_t)*morph,(int32_t)*depth,(int32_t)*cutoff,
                   space?(int32_t)*space:0};
 for(uint32_t i=0;i<ETCH_MATRIX_ROUTES;i++){
  const struct etch_mod_route *r=&m->route[i];
  if(!r->source||!r->amount||(r->target==3u&&!space))continue;
  int32_t src=0;
  switch(r->source){
   case 1:src=(int32_t)velocity*32767/127;break;
   case 2:src=envelope>32767u?32767:(int32_t)envelope;break;
   case 3:src=wave_c;break;
   case 4:src=living>32767u?32767:(int32_t)living;break;
   case 5:src=(int32_t)pressure*32767/127;break;
   case 6:src=(int32_t)note*32767/127;break;
   default:break;
  }
  values[r->target]+=(src*etch_matrix_signed(r->amount))/400;
 }
 for(uint8_t j=0;j<4u;j++){
  int32_t x=values[j];
  uint16_t out=(uint16_t)(x<0?0:x>32767?32767:x);
  if(j==0u)*morph=out;
  else if(j==1u)*depth=out;
  else if(j==2u)*cutoff=out;
  else if(space)*space=out;
 }
}
static inline uint16_t etch_matrix_apply_ex(const struct etch_mod_matrix *m,
 uint8_t target,uint16_t base,uint8_t velocity,uint16_t envelope,
 int16_t wave_c,uint16_t living,uint8_t pressure,uint8_t note){
 if(!etch_matrix_active(m))return base;
 return etch_matrix_apply_active_ex(m,target,base,velocity,envelope,
                                  wave_c,living,pressure,note);
}
/* Old callers retain exact pressure=0 behaviour. */
static inline uint16_t etch_matrix_apply(const struct etch_mod_matrix *m,
 uint8_t target,uint16_t base,uint8_t velocity,uint16_t envelope,
 int16_t wave_c,uint16_t living){
 return etch_matrix_apply_ex(m,target,base,velocity,envelope,wave_c,living,0,64);
}
