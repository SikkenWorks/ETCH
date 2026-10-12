/* SPDX-License-Identifier: GPL-3.0-only
 * Capacity-limited independent pattern snapshots for 32 patch slots.
 * A packed record is always validated before replacing the previous copy.
 */
#pragma once
#include <stdint.h>
#include "native_performance.h"
#include "native_patch.h"
/* In the default, conservatively mapped 20 KiB build retain the legacy
 * archive memory limit. The lab-only extended dual-bank build has enough
 * verified HOST capacity for a 24 KiB archive. On-device ownership remains
 * unverified. Records retain the identical on-disk PJT10 encoding. */
#ifndef ETCH_P04_EXPANDED
#define ETCH_P04_EXPANDED 0
#endif
#define ETCH_PATTERN_ARCHIVE_BYTES (ETCH_P04_EXPANDED ? 24000u : 16000u)
struct etch_pattern_bank {
 uint16_t lengths[ETCH_PATCH_SLOTS];
 uint8_t data[ETCH_PATTERN_ARCHIVE_BYTES];
 uint16_t used;
};
void etch_pattern_bank_clear(struct etch_pattern_bank *bank);
/* Returns 0 on success, -1 invalid, -2 would exceed archive capacity. */
int etch_pattern_bank_capture(struct etch_pattern_bank *bank,uint8_t slot,const struct etch_native_pattern *pattern);
/* 1 = found, 0 = empty (clears output), -1 = corrupt; destination not altered on corruption. */
int etch_pattern_bank_fetch(const struct etch_pattern_bank *bank,uint8_t slot,struct etch_native_pattern *out);
int etch_pattern_bank_validate(const struct etch_pattern_bank *bank);
