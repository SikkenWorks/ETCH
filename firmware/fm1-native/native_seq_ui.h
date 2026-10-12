/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V0.43: testable FM1 sequencer cursor/key-LED model.
 * Pure UI helpers: never touch audio state or physical GPIO.
 */
#pragma once
#include <stdint.h>
#include "native_performance.h"
struct etch_seq_key_view {uint32_t triggers,holds,selected;};
/* Keys 0..15 represent the current 16-step GRID page. */
uint16_t etch_seq_grid_key_select(uint16_t current,uint16_t length,uint8_t key);
struct etch_seq_grid_view {uint16_t triggers,holds,locks,selected;};
int etch_seq_grid_view(const struct etch_native_pattern *p,uint16_t length,
                       uint16_t selected,struct etch_seq_grid_view *out);
uint8_t etch_seq_note_cursor(uint8_t count,uint8_t cursor);
/* FM1 key zero is MIDI 53 (F3), before octave offset. All masks are 27-bit. */
int etch_seq_key_view(const struct etch_native_pattern *p,uint16_t step,
                     int8_t octave,uint8_t cursor,struct etch_seq_key_view *out);
/* Atomic lane-specific erase. bits: 1=triggers, 2=hold pitches, 4=locks.
 * Never affects unrequested lanes; 0 means rejected/invalid. */
uint8_t etch_seq_step_clear(struct etch_native_pattern *p,uint16_t step,uint8_t mask);

/* NOTES musical-key selection: 1..8 means selected existing/new note; 0 rejected.
 * Existing notes are SELECTED, never removed by an ordinary key press.
 * REC removes only the selected note. */
uint8_t etch_seq_note_key_select(struct etch_native_pattern *p,uint16_t step,
 uint8_t pitch,uint8_t *cursor);
