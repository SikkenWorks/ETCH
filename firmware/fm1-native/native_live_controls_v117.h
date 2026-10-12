/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.17: explicit four-encoder mapping for A/B pre-morph FX and Evolution.
 * Keep control deltas independent of waveform drawing and avoid dead detents
 * from roundtripping small Q15 percentages through 6/7-bit preset packing.
 */
#pragma once
#include <stdint.h>
#include "native_fx.h"
#include "native_evolution.h"

static inline uint8_t etch_v117_bound8(int32_t value,uint8_t max){
 return (uint8_t)(value<0?0:value>max?max:value);
}
static inline uint8_t etch_v117_pre_slot(uint8_t slot,int32_t k1){
 return etch_v117_bound8((int32_t)slot+k1,1u);
}
static inline void etch_v117_pre_turn(struct etch_fx_settings *fx,uint8_t wave,
                                      uint8_t slot,int32_t k2,int32_t k3,int32_t k4){
 if(!fx||wave>1u||slot>1u)return;
 uint16_t packed=fx->pre[wave][slot];
 uint8_t old_type=(uint8_t)(packed&7u);
 uint8_t type=etch_v117_bound8((int32_t)old_type+k2,5u);
 uint8_t depth=(uint8_t)((packed>>3)&127u);
 uint8_t mix=(uint8_t)((packed>>10)&63u);
 /* Enabling a new effect from OFF must be audible without K4 having to be
  * turned first. Explicitly saved MIX=0 on an active effect remains dry. */
 if(type!=0u&&old_type==0u&&!mix)mix=32u;
 depth=etch_v117_bound8((int32_t)depth+k3*2,127u);
 mix=etch_v117_bound8((int32_t)mix+k4,63u);
 fx->pre[wave][slot]=(uint16_t)((uint16_t)type|((uint16_t)depth<<3)|((uint16_t)mix<<10));
}
static inline void etch_v117_evolution_turn(struct etch_evolution_settings *e,
                                            int32_t k1,int32_t k2,int32_t k3,int32_t k4,
                                            uint8_t rec_pressed){
 if(!e)return;
 e->depth=etch_v117_bound8((int32_t)e->depth+k1*5,100u);
 e->rate=etch_v117_bound8((int32_t)e->rate+k2*5,100u);
 e->mode=etch_v117_bound8((int32_t)e->mode+k3,2u);
 e->sync=etch_v117_bound8((int32_t)e->sync+k4,2u);
 if(rec_pressed)e->depth=e->depth?0u:60u;
}
/* LCD footer cache keys. Always include these in the footer fingerprint.
 * Otherwise parameter updates are applied to audio but the screen keeps
 * displaying the previous OFF/DEPTH/RATE/SHAPE/SYNC values. */
#define ETCH_V117_EXTRA_FOOTER_VALUES(fx,e,slot) \
 (fx).pre[0][0],(fx).pre[0][1],(fx).pre[1][0],(fx).pre[1][1], \
 (fx).c_fx,(slot),(e).depth,(e).rate,(e).mode,(e).sync
