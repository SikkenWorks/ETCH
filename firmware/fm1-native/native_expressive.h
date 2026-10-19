/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* V1.40 phase 1: velocity response selectable by runtime controls.
 * 0 linear (bit exact V1.39); 1 soft; 2 hard. Integer-only and bounded.
 * Velocity 0 is reserved for MIDI note-off by the MIDI dispatcher. */
static inline uint8_t etch_expressive_velocity(uint8_t velocity, uint8_t curve){
 uint32_t v=velocity>127u?127u:velocity;
 if(curve==1u){ /* concave: soft notes remain articulate */
  return (uint8_t)((v*(254u-v)+126u)/127u);
 }
 if(curve==2u){ /* convex: greater dynamic range for strong strikes */
  return (uint8_t)((v*v+63u)/127u);
 }
 return (uint8_t)v;
}
