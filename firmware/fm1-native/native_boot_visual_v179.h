/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH 1.79: no-heap, no-flash boot animation and workflow prompt decisions.
 * FM1: all splash drawing happens before audio playback starts.
 */
#ifndef ETCH_BOOT_VISUAL_V179_H
#define ETCH_BOOT_VISUAL_V179_H
#include <stdint.h>
#define ETCH_BOOT_GLITCH_FRAMES 9u
#define ETCH_BOOT_FRAME_DELAY_MS 42u
#define ETCH_BOOT_BRAND_HOLD_MS 1000u
#define ETCH_BOOT_WORKFLOW_SIMPLE 0u
#define ETCH_BOOT_WORKFLOW_ADVANCED 1u
static inline uint8_t etch_boot_mode_pick(uint8_t old,int32_t k1,int32_t select){
 int32_t delta=k1+select;
 if(delta<0)return ETCH_BOOT_WORKFLOW_SIMPLE;
 if(delta>0)return ETCH_BOOT_WORKFLOW_ADVANCED;
 return old;
}
/* Deterministic frame-level jitter; no random state and no DSP heap usage. */
static inline int8_t etch_boot_rgb_shift(uint8_t frame){
 static const int8_t shifts[ETCH_BOOT_GLITCH_FRAMES]={-3,3,-2,2,-1,3,-2,1,0};
 return shifts[frame%ETCH_BOOT_GLITCH_FRAMES];
}
#endif
