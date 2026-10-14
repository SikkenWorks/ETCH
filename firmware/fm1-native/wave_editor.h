/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "native_wave_fx.h"
#define ETCH_EDIT_POINTS 128
struct etch_native_bank {int16_t table[3][6][512];};
struct etch_wave_editor {
 int16_t points[3][128];
 struct etch_wave_fx_params fx[3];
 int16_t history[16][3][128],redo[16][3][128];
 uint8_t undo_count,undo_head,redo_count;
 uint8_t wave,cursor,has_undo,dirty;
 uint32_t revision;
};
void etch_editor_init(struct etch_wave_editor *e);
void etch_editor_move(struct etch_wave_editor *e,int32_t left,int32_t right,int32_t up,int32_t down);
void etch_editor_select(struct etch_wave_editor *e,int32_t delta);
void etch_editor_undo(struct etch_wave_editor *e);
void etch_editor_compile(const struct etch_wave_editor *e,struct etch_native_bank *out);

void etch_editor_checkpoint(struct etch_wave_editor *e);
void etch_editor_redo(struct etch_wave_editor *e);

/* Tool 0=line,1=smooth,2=mirror,3=invert,4=normalize,5=flatten,
 * 6=random,7=sine,8=triangle,9=saw,10=square,11=rotate. */
void etch_editor_tool(struct etch_wave_editor *e,uint8_t tool,int32_t parameter);
void etch_editor_bake_fx(struct etch_wave_editor *e);

/* 32 intuitive editable grips mapped to the existing 128-point wave storage. */
void etch_editor_move_xy32(struct etch_wave_editor *e,int32_t xturns,int32_t yturns);

/* Editable harmonic sculpt: K3 selects 2..16 and K4 applies +/- gain. */
void etch_editor_add_partial(struct etch_wave_editor *e,uint8_t harmonic,int32_t turns);

/* V1.14: combine current loaded Wave A/B into a new editable Wave C.
 * 16-step Undo works; no flash write until user explicitly saves.
 * mix/mutate: 0..100, seed: user-supplied deterministic variation. */
void etch_editor_genetics(struct etch_wave_editor *e,uint8_t mix,uint8_t mutate,uint8_t seed);

/* V1.24 SPECTRAL ETCH. These operations modify actual stored waveform points,
 * checkpoint existing Undo, and are never called from the audio ISR. */
void etch_editor_spectral_partial(struct etch_wave_editor *e,uint8_t harmonic,int32_t turns);
void etch_editor_spectral_tilt(struct etch_wave_editor *e,int32_t turns);
uint16_t etch_editor_spectral_level(const struct etch_wave_editor *e,uint8_t harmonic);
uint16_t etch_editor_spectral_brightness(const struct etch_wave_editor *e);
