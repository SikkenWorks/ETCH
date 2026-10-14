/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.30: independent, bounded controls for advanced synth pages.
 * This is the actual FM1 input path, shared with host regression tests.
 */
#pragma once
#include <stdint.h>
#include "native_organic.h"
#include "native_living.h"
#include "native_mod_matrix.h"
static inline uint8_t etch_v130_edit_pct(uint8_t old,int32_t steps){
 int64_t n=(int64_t)old+steps;
 return (uint8_t)(n<0?0:n>100?100:n);
}
static inline uint8_t etch_v130_bpm(uint8_t old,int32_t steps){
 int64_t n=(int64_t)old+steps;
 return (uint8_t)(n<30?30:n>240?240:n);
}
static inline void etch_v130_organic(struct etch_organic_settings *s,int32_t k1,int32_t k2,int32_t k3,int32_t k4){
 s->drift=etch_v130_edit_pct(s->drift,k1);
 s->rate=etch_v130_edit_pct(s->rate,k2);
 s->phase=etch_v130_edit_pct(s->phase,k3);
 s->variance=etch_v130_edit_pct(s->variance,k4);
}
static inline void etch_v130_living(struct etch_living_settings *s,int32_t k1,int32_t k2,int32_t k3,int32_t k4){
 s->depth=etch_v130_edit_pct(s->depth,k1);
 s->rate=etch_v130_edit_pct(s->rate,k2);
 s->color=etch_v130_edit_pct(s->color,k3);
 s->vary=etch_v130_edit_pct(s->vary,k4);
}
static inline uint8_t etch_v130_route_slot(uint8_t old,int32_t steps){
 int64_t n=(int64_t)old+steps;
 return (uint8_t)(n<0?0:n>=ETCH_MATRIX_ROUTES?ETCH_MATRIX_ROUTES-1u:n);
}
static inline void etch_v130_matrix(struct etch_mod_matrix *m,uint8_t *slot,int32_t k1,int32_t k2,int32_t k3,int32_t k4){
 *slot=etch_v130_route_slot(*slot,k1);
 struct etch_mod_route *r=&m->route[*slot];
 int64_t src=(int64_t)r->source+k2,dst=(int64_t)r->target+k3;
 r->source=(uint8_t)(src<0?0:src>4?4:src);
 r->target=(uint8_t)(dst<0?0:dst>3?3:dst);
 r->amount=etch_v130_edit_pct(r->amount,k4);
}
