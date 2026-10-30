/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH DRAW input isolation: never interpret a piano-key transition as a
 * waveform-edit encoder detent. No input ISR, audio path or flash writes.
 */
#pragma once
#include <stdint.h>

#define ETCH_DRAW_KEY_GUARD_MS 65u

struct etch_draw_key_guard {
 uint32_t last_notes;
 uint32_t transition_ms;
 uint8_t guarding;
};

static inline void etch_draw_key_guard_reset(struct etch_draw_key_guard *g,uint32_t notes){
 g->last_notes=notes&0x07ffffffu;
 g->transition_ms=0u;
 g->guarding=0u;
}

/* A physical key change can inject a false encoder detent into the shared
 * FM1 key/encoder matrix. The low-level scan has no reliable way to know
 * which encoder pulse was genuine. Drop draw-control deltas only during the
 * short key transition, never while a key is steadily held. Note-on and
 * note-off themselves continue through the unmodified audio path.
 * note_press_edges captures very fast taps between 25 ms UI polls. */
static inline uint8_t etch_draw_key_guard_tick(struct etch_draw_key_guard *g,
                                               uint32_t notes,uint32_t note_press_edges,
                                               uint32_t now_ms){
 notes&=0x07ffffffu;
 if(notes!=g->last_notes || (note_press_edges&0x07ffffffu)){
  g->last_notes=notes;
  g->transition_ms=now_ms;
  g->guarding=1u;
 }
 if(!g->guarding)return 0u;
 if((uint32_t)(now_ms-g->transition_ms)<ETCH_DRAW_KEY_GUARD_MS)return 1u;
 g->guarding=0u;
 return 0u;
}
