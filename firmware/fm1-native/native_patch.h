/* SPDX-License-Identifier: GPL-3.0-only
 * Compact on-device patch record, 32 persistent slots.
 * MASTER is intentionally not stored.
 */
#pragma once
#include <stdint.h>
#define ETCH_PATCH_SLOTS 32u
#define ETCH_NAME_CHARS 12u
struct etch_native_patch {
 char name[ETCH_NAME_CHARS];
 uint8_t valid,slot_a,slot_b,slot_c,mod_mode,mod_target,mod_ratio,filter_type;
 uint8_t arp_mode,seq_length,tempo,flags;
 uint16_t morph,depth,attack_ms,decay_ms,release_ms,sustain;
 uint16_t cutoff,resonance,drive,chorus,delay,reverb;
};
_Static_assert(sizeof(struct etch_native_patch)<=48,"patch is compact");
