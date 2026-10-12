/* SPDX-License-Identifier: GPL-3.0-only
 * Nonblocking USB-MIDI mirror of real ARP/SEQ voices, no MIDI-IN echo.
 * Called once per rendered audio block; notes-off always have priority.
 */
#pragma once
#include <stdint.h>
#include "audio_core.h"
typedef int (*etch_perf_emit)(uint32_t,void*);
struct etch_perf_midi {
 uint8_t sent[ETCH_NATIVE_VOICES];
 uint8_t note[ETCH_NATIVE_VOICES];
 /* A voice slot can retrigger the same pitch with a new allocation birth.
  * Tracking only pitch loses that MIDI note-off/on pair. */
 uint32_t birth[ETCH_NATIVE_VOICES];
 uint8_t transport;
 uint32_t clock_phase;
};
void etch_perf_midi_reset(struct etch_perf_midi *m);
void etch_perf_midi_update(struct etch_perf_midi *m,const struct etch_native_audio *audio,
 const struct etch_native_controls *controls,uint8_t connected,uint32_t frames,
 etch_perf_emit emit,void *context);
void etch_perf_midi_flush(struct etch_perf_midi *m,etch_perf_emit emit,void *context);
