/* SPDX-License-Identifier: GPL-3.0-only
 * Gapless patch-pattern staging: the audio callback reads the published bank
 * until the main loop has validated a complete replacement in the other bank.
 * Only the caller publishes the new pointer with IRQs masked.
 */
#pragma once
#include <stdint.h>
#include "native_pattern_bank.h"
struct etch_patch_swap {
 struct etch_native_pattern patterns[2];
 uint8_t active;
};
static inline struct etch_native_pattern *etch_patch_active(struct etch_patch_swap *s){return &s->patterns[s->active&1u];}
static inline struct etch_native_pattern *etch_patch_staging(struct etch_patch_swap *s){return &s->patterns[(s->active^1u)&1u];}
/* Returns 1 (loaded), 0 (empty), or -1 (corrupt). No active pattern change. */
static inline int etch_patch_prepare(struct etch_patch_swap *s,
  const struct etch_pattern_bank *archive,uint8_t slot){
 if(!s||!archive||slot>=ETCH_PATCH_SLOTS)return -1;
 return etch_pattern_bank_fetch(archive,slot,etch_patch_staging(s));
}
/* Main loop must publish the returned pointer with audio IRQs masked. */
static inline struct etch_native_pattern *etch_patch_commit(struct etch_patch_swap *s){
 s->active^=1u;return etch_patch_active(s);
}
