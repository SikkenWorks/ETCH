/* SPDX-License-Identifier: GPL-3.0-only
 * Bounded, queue-backpressure-aware panel MIDI event generator.
 * USB event packets: CIN | status<<8 | note<<16 | velocity<<24.
 */
#pragma once
#include <stdint.h>
struct etch_panel_midi { uint8_t active[27], note[27]; };
typedef int (*etch_panel_emit)(uint32_t packet,void *context);
void etch_panel_midi_reset(struct etch_panel_midi *m);
void etch_panel_midi_update(struct etch_panel_midi *m,uint32_t keys,int8_t octave,
                           uint8_t connected,etch_panel_emit emit,void *context);
