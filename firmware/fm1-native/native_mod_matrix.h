/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.26 MOD MATRIX — four bounded, project-scoped routes.
 * No allocations or floating point in the audio callback.
 */
#pragma once
#include <stdint.h>
#define ETCH_MATRIX_ROUTES 4u
/* sources: 0 OFF, 1 VELOCITY, 2 ENVELOPE, 3 WAVE C, 4 LIVING */
/* targets: 0 MORPH, 1 C DEPTH, 2 FILTER, 3 SPACE */
struct etch_mod_route {uint8_t source,target,amount;};
struct etch_mod_matrix {struct etch_mod_route route[ETCH_MATRIX_ROUTES];};
static inline uint8_t etch_matrix_valid(const struct etch_mod_matrix *m){
 if(!m)return 0;
 for(uint32_t i=0;i<ETCH_MATRIX_ROUTES;i++)if(m->route[i].source>4u ||
     m->route[i].target>3u ||m->route[i].amount>100u)return 0;
 return 1;
}
static inline uint8_t etch_matrix_active(const struct etch_mod_matrix *m){
 if(!etch_matrix_valid(m))return 0;
 for(uint32_t i=0;i<ETCH_MATRIX_ROUTES;i++)if(m->route[i].source&&m->route[i].amount)return 1;
 return 0;
}
/* Sources are normalized to [-32767,32767]. VELOCITY/ENVELOPE and
 * LIVING are naturally unipolar (0..32767); Wave C is bipolar.
 * Per-route maximum is +/-25 percent of the destination's range.
 * Saturation is applied AFTER accumulation, not at each route. */
static inline uint16_t etch_matrix_apply(const struct etch_mod_matrix *m,
 uint8_t target,uint16_t base,uint8_t velocity,uint16_t envelope,
 int16_t wave_c,uint16_t living){
 if(!etch_matrix_valid(m))return base;
 int32_t result=base;
 for(uint32_t i=0;i<ETCH_MATRIX_ROUTES;i++){
  const struct etch_mod_route *r=&m->route[i];
  if(!r->source||!r->amount||r->target!=target)continue;
  int32_t src=r->source==1?(int32_t)velocity*32767/127:
   r->source==2?(int32_t)envelope:
   r->source==3?(int32_t)wave_c:(int32_t)living;
  result+=(src*(int32_t)r->amount)/400;
 }
 return (uint16_t)(result<0?0:result>32767?32767:result);
}
