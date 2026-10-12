/* SPDX-License-Identifier: GPL-3.0-only
 * The 12 non-octave function buttons remain lit. Octaves indicate offset.
 * Musical keyboard LEDs still follow active notes and sequencer feedback.
 */
#pragma once
#include <stdint.h>
#include "native_performance.h"
static inline uint8_t etch_led_function(uint8_t button){return button>=2u && button<14u;}
static inline uint8_t etch_led_octave(uint8_t button,int8_t octave,uint32_t pressed){
 return (uint8_t)(button==0u?(octave<0 || (pressed&1u)):(octave>0 || (pressed&2u)));
}

/* Highlight the ACTUALLY AUDIBLE scale-locked key as well as the physical
 * key.  Audio core uses exactly the same base note (53 + key + octave*12)
 * and etch_quantize_note() on every note-on.  Rebuild from the held-key
 * bitmap each UI tick: two fingers snapping to the same pitch cannot turn
 * each other's target LED off when one finger is released.
 *
 * These bits describe only pitches physically present on the 27-key FM1.
 * Do not wrap octaves or pretend an off-panel pitch is another key.
 * This is a UI-only computation, NEVER run from the audio callback.
 */
static inline uint32_t etch_led_keylock_targets(uint32_t held_keys,int8_t octave,
                                                const struct etch_harmony_settings *h){
 if(!h||h->keylock!=2u)return 0u; /* OFF / GUIDE must not remap. */
 held_keys&=0x07ffffffu;
 const int base=53+12*(int)octave;
 uint32_t targets=0u;
 for(unsigned k=0;k<27u;k++){
  if(!(held_keys&(1u<<k)))continue;
  int note=base+(int)k;
  if(note<0)note=0;
  if(note>127)note=127;
  const int tuned=(int)etch_quantize_note((uint8_t)note,h);
  const int physical=tuned-base;
  if(physical>=0&&physical<27)targets|=(uint32_t)1u<<(unsigned)physical;
 }
 return targets;
}

/* GUIDE lights every physical FM1 key within the chosen scale, not only
 * the notes currently being pressed. 27 keys map to consecutive MIDI
 * notes 53..79 at octave zero. Key signatures repeat naturally on each
 * visible octave, while octave adjustment keeps the mapping consistent.
 * OFF and LOCK do not enable the static scale guide.
 * Pure control-thread helper: never called from the realtime audio ISR. */
static inline uint32_t etch_led_scale_guide_targets(int8_t octave,
                                         const struct etch_harmony_settings *h){
 if(!h||h->keylock!=1u)return 0u;
 struct etch_harmony_guide guide;
 if(etch_harmony_guide_build(h,60u,&guide))return 0u;
 const int base=53+12*(int)octave;
 uint32_t result=0u;
 for(uint8_t key=0;key<27u;key++){
  const int note=base+(int)key;
  if(note>=0&&note<=127&&(guide.scale_mask&(uint16_t)(1u<<(note%12))))
   result|=(uint32_t)1u<<key;
 }
 return result;
}
