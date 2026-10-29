/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.79. LCD-dirty decision for the storage ownership modal.
 * Pure state logic: no I/O, flash writes, interrupts or allocations.
 */
#ifndef ETCH_STORAGE_PROMPT_PAINT_H
#define ETCH_STORAGE_PROMPT_PAINT_H
#include <stdint.h>
#include "native_storage_boot_prompt.h"
#define ETCH_STORAGE_PROGRESS_STEPS 12u
struct etch_storage_prompt_paint {
 uint8_t valid,phase,yes,busy,status,progress;
};
static inline uint8_t etch_storage_prompt_progress(
 const struct etch_storage_boot_prompt *p,uint32_t buttons,uint32_t now){
 if(p->phase!=ETCH_STORAGE_PROMPT_CONFIRM||!p->save_released||
    !p->save_held_at||!(buttons&(1u<<9)))return 0u;
 uint32_t elapsed=(uint32_t)(now-p->save_held_at);
 if(elapsed>=ETCH_STORAGE_CONFIRM_HOLD_MS)return ETCH_STORAGE_PROGRESS_STEPS;
 return (uint8_t)((elapsed*ETCH_STORAGE_PROGRESS_STEPS)/ETCH_STORAGE_CONFIRM_HOLD_MS);
}
static inline uint8_t etch_storage_prompt_paint_needed(
 struct etch_storage_prompt_paint *cache,uint8_t phase,uint8_t yes,
 uint8_t busy,uint8_t status,uint8_t progress,uint8_t force){
 const uint8_t dirty=(uint8_t)(force||!cache->valid||cache->phase!=phase||
    cache->yes!=yes||cache->busy!=busy||cache->status!=status||
    cache->progress!=progress);
 if(!dirty)return 0u;
 cache->valid=1u;cache->phase=phase;cache->yes=yes;cache->busy=busy;
 cache->status=status;cache->progress=progress;
 return 1u;
}
#endif
