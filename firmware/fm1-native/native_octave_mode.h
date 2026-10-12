/* SPDX-License-Identifier: GPL-3.0-only
 * OCT- + OCT+ = one voice-mode toggle per complete chord.
 * Single button octave actions commit on release, so pressing the second
 * octave button a moment later cannot accidentally shift octave first.
 * This does not alter OCT- power-on recovery handling.
 */
#pragma once
#include <stdint.h>
enum etch_octave_action {ETCH_OCT_NONE=0,ETCH_OCT_MINUS=1,ETCH_OCT_PLUS=2,ETCH_OCT_TOGGLE=3};
struct etch_octave_chord {uint8_t last,combo_latched;};
static inline enum etch_octave_action etch_octave_scan(struct etch_octave_chord *s,uint32_t buttons){
 uint8_t now=(uint8_t)(buttons&3u),before=s->last;
 s->last=now;
 if(now==3u){
  if(!s->combo_latched){s->combo_latched=1;return ETCH_OCT_TOGGLE;}
  return ETCH_OCT_NONE;
 }
 if(s->combo_latched){if(!now)s->combo_latched=0;return ETCH_OCT_NONE;}
 /* Require a full release of a solitary OCT button before stepping. */
 if(!now&&before==1u)return ETCH_OCT_MINUS;
 if(!now&&before==2u)return ETCH_OCT_PLUS;
 return ETCH_OCT_NONE;
}
