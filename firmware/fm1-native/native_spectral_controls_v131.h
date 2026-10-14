/* SPDX-License-Identifier: GPL-3.0-only
 * Spectral editor UI: distinct persistent K3/K4 edit counters.
 * 50 is neutral and both controls are independently bounded 0..100.
 * The waveform changes are destructive and tracked by the editor's undo stack.
 */
#pragma once
#include <stdint.h>
#include "wave_editor.h"
static inline uint8_t etch_spectral_bound(int32_t n){return (uint8_t)(n<0?0:n>100?100:n);}
static inline void etch_spectral_controls_turn(struct etch_wave_editor *e,uint8_t harmonic,
 uint8_t *gain,uint8_t *tilt,int32_t k3,int32_t k4){
 if(!e||!gain||!tilt)return;
 if(k3){
  uint8_t next=etch_spectral_bound((int32_t)*gain+k3);
  int32_t delta=(int32_t)next-(int32_t)*gain;
  *gain=next;
  if(delta)etch_editor_spectral_partial(e,harmonic,delta);
 }
 if(k4){
  uint8_t next=etch_spectral_bound((int32_t)*tilt+k4);
  int32_t delta=(int32_t)next-(int32_t)*tilt;
  *tilt=next;
  if(delta)etch_editor_spectral_tilt(e,delta);
 }
}
