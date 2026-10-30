/* SPDX-License-Identifier: GPL-3.0-only. V1.84 attack smoothing + VHS integrity. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "native_attack_smoothing_v184.h"
#include "native_vhs_tape_v184.h"
int main(void){
 assert(ETCH_ATTACK_SAMPLES==256u);
 assert(etch_attack_gain_q8(256u)==0u);
 assert(etch_attack_gain_q8(0u)==256u);
 uint16_t last=0u;
 for(uint32_t k=0;k<=256u;k++){
  uint16_t gain=etch_attack_gain_q8((uint16_t)(256u-k));
  assert(gain>=last && gain<=256u); last=gain;
  int32_t s=etch_attack_apply(30000,(uint16_t)(256u-k));
  assert(s>=0 && s<=30000);
  assert(etch_attack_apply(-30000,(uint16_t)(256u-k))<=0);
 }
 assert(etch_attack_gain_q8(255u)<=1u);
 assert(etch_attack_gain_q8(1u)>=255u);
 assert(etch_attack_apply(10000,0u)==10000);
 for(uint32_t frame=0;frame<9u;frame++){
  for(uint32_t band=0;band<2u;band++){
   int shift=etch_vhs_band_shift((uint8_t)frame,(uint8_t)band);
   assert(shift>=-8 && shift<=8);
   assert(etch_vhs_band_shift((uint8_t)(frame+9u),(uint8_t)band)==shift);
  }
  for(uint32_t strip=0;strip<3u;strip++){
   uint32_t y=etch_vhs_dropout_y((uint8_t)frame,(uint8_t)strip);
   assert(y>=79u && y<224u);
  }
 }
 puts("PASS V1.84: cubic attack onset, zero overhead in steady state, deterministic bounded VHS tearing");
 return 0;
}
