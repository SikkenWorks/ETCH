/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.78: read-only first-boot storage ownership question.
 * Pure control-state machine. No flash access, I/O or implicit erases.
 */
#pragma once
#include <stdint.h>
#include "wave_store.h"

enum etch_storage_prompt_phase {
 ETCH_STORAGE_PROMPT_CLOSED=0,
 ETCH_STORAGE_PROMPT_CHOOSE=1,
 ETCH_STORAGE_PROMPT_CONFIRM=2,
 ETCH_STORAGE_PROMPT_FAILED=3,
 ETCH_STORAGE_PROMPT_SUCCESS=4
};
enum etch_storage_prompt_action {
 ETCH_STORAGE_PROMPT_NONE=0,
 ETCH_STORAGE_PROMPT_KEEP=1,
 ETCH_STORAGE_PROMPT_FORMAT=2
};
#define ETCH_STORAGE_CONFIRM_HOLD_MS 3000u
struct etch_storage_boot_prompt {
 uint32_t save_held_at;
 uint8_t phase,yes,save_released;
};
static inline void etch_storage_prompt_open(struct etch_storage_boot_prompt *p,
 int storage_status,uint8_t recovery_normal,uint8_t storage_enabled){
 p->phase=(uint8_t)((storage_status==ETCH_STORE_FOREIGN&&recovery_normal&&storage_enabled)?
 ETCH_STORAGE_PROMPT_CHOOSE:ETCH_STORAGE_PROMPT_CLOSED);
 p->yes=0u; /* NO is ALWAYS the default. */
 p->save_released=0u;
 p->save_held_at=0u;
}
static inline enum etch_storage_prompt_action etch_storage_prompt_input(
 struct etch_storage_boot_prompt *p,int32_t choose,int32_t select,
 uint32_t pressed,uint32_t buttons,uint32_t now){
 /* HOME button 8: keep all data; never erase. */
 if(!p->phase||p->phase==ETCH_STORAGE_PROMPT_SUCCESS)return ETCH_STORAGE_PROMPT_NONE;
 if(pressed&(1u<<8)){
  p->phase=ETCH_STORAGE_PROMPT_CLOSED;p->save_held_at=0u;
  return ETCH_STORAGE_PROMPT_KEEP;
 }
 if(p->phase==ETCH_STORAGE_PROMPT_CHOOSE){
  if(choose||select)p->yes=(uint8_t)((choose+select)>0);
  if(pressed&(1u<<3)){ /* SEL: enter explicit second confirmation. */
   if(!p->yes){p->phase=ETCH_STORAGE_PROMPT_CLOSED;return ETCH_STORAGE_PROMPT_KEEP;}
   p->phase=ETCH_STORAGE_PROMPT_CONFIRM;p->save_held_at=0u;p->save_released=0u;
  }
 }else if(p->phase==ETCH_STORAGE_PROMPT_CONFIRM){
  /* Selection can never erase by itself. Require a fresh 3s SAVE hold. */
  if(!(buttons&(1u<<9))){p->save_released=1u;p->save_held_at=0u;}
  else if(p->save_released){
   if(!p->save_held_at)p->save_held_at=now?now:1u;
   if((uint32_t)(now-p->save_held_at)>=ETCH_STORAGE_CONFIRM_HOLD_MS){
    p->save_held_at=0u;
    p->save_released=0u;
    return ETCH_STORAGE_PROMPT_FORMAT;
   }
  }
 }else if(p->phase==ETCH_STORAGE_PROMPT_FAILED){
  /* A format error is never retried automatically. HOME can exit. */
 }
 return ETCH_STORAGE_PROMPT_NONE;
}
