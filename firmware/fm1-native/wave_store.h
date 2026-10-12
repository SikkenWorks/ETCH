/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "native_patch.h"
#include <stddef.h>
#define ETCH_WAVE_SLOTS 64
#define ETCH_STORE_BASE 0x97000u
#define ETCH_STORE_BANK_BYTES 0x5000u
#define ETCH_STORE_END (ETCH_STORE_BASE+2u*ETCH_STORE_BANK_BYTES)
/* P04 optional expansion. 64 KiB per A/B copy. Separate from legacy snapshots
 * (0x97000..0xA1000), application (..0x93000) and loader staging
 * (0xE0000..0xE5000). Felucca uses this address span for user samples:
 * NEVER erase existing/foreign data; this is only safe on independently
 * verified hardware with an erased region and a full stock backup.
 */
#ifndef ETCH_P04_EXPANDED
#define ETCH_P04_EXPANDED 0
#endif
/* Experimental non-destructive spare-area store. The FM1/Felucca map lists
 * 0xA1000..0xE0000 as Felucca user/sample storage. Claim ONLY after a full read-only blank
 * scan of both banks, never erase or migrate unknown records automatically.
 * The spare area is claimed only if every sector is erased AND no Felucca
 * FSMP sample-owner marker exists at 0xA0000. OTA, autosave and globals
 * are never writable through this storage route. */
#ifndef ETCH_STORE_SPARE_ENABLED
#define ETCH_STORE_SPARE_ENABLED 0
#endif
#define ETCH_STORE_SPARE_BASE 0xA1000u
#define ETCH_STORE_SPARE_BANK_BYTES 0x5000u
#define ETCH_STORE_SPARE_END (ETCH_STORE_SPARE_BASE+2u*ETCH_STORE_SPARE_BANK_BYTES)
_Static_assert(ETCH_STORE_SPARE_END <= 0xE0000u,"spare store must not touch OTA, autosave or globals");
/* Dedicated ETCH project bank pair within Felucca sample slots 2 and 3.
 * This is NOT automatically free: both sectors belong to Felucca until a
 * user explicitly transfers ownership in SYSTEM / STORAGE. The takeover
 * irreversibly destroys only sample slots 2 and 3 (0xB4000..0xDC000).
 * Legacy ETCH A/B and Felucca project/sample 1 sectors are never erased.
 * Two 80-KiB banks allow four bounded project records in a verified A/B log.
 */
#ifndef ETCH_STORE_OWNED_ENABLED
#define ETCH_STORE_OWNED_ENABLED 0
#endif
#define ETCH_STORE_OWNED_BASE 0xB4000u
#define ETCH_STORE_OWNED_BANK_BYTES 0x14000u
#define ETCH_STORE_OWNED_END (ETCH_STORE_OWNED_BASE+2u*ETCH_STORE_OWNED_BANK_BYTES)
_Static_assert(ETCH_STORE_OWNED_END==0xDC000u,"storage must stop before Felucca user presets and OTA");
#define ETCH_STORE_EXT_BASE 0xA1000u
#define ETCH_STORE_EXT_BANK_BYTES 0x10000u
#define ETCH_STORE_EXT_END (ETCH_STORE_EXT_BASE+2u*ETCH_STORE_EXT_BANK_BYTES)
#if ETCH_P04_EXPANDED
_Static_assert(ETCH_STORE_EXT_BASE >= ETCH_STORE_END, "legacy and P04 regions must not overlap");
_Static_assert(ETCH_STORE_EXT_END <= 0xE0000u, "P04 must not touch OTA staging");
#endif
struct etch_wave_library {
 int16_t slot[64][128];
 int16_t active[3][128];
 uint32_t valid[2];
 uint8_t selection[3];uint8_t reserved;
 /* Append-only v2 payload: first 17164 bytes exactly match v1. */
 char wave_names[64][ETCH_NAME_CHARS];
 struct etch_native_patch patches[ETCH_PATCH_SLOTS];
 uint8_t sequence[64];
};
#define ETCH_STORE_V1_BYTES offsetof(struct etch_wave_library,wave_names)
_Static_assert(ETCH_STORE_V1_BYTES==17164,"v1 layout must never change");
struct etch_store_io {int (*read)(uint32_t,void*,uint32_t);int (*erase)(uint32_t);int (*program)(uint32_t,const void*,uint32_t);};
enum {ETCH_STORE_OK=0,ETCH_STORE_EMPTY=1,ETCH_STORE_FOREIGN=-2,ETCH_STORE_IO=-3,ETCH_STORE_VERIFY=-4,ETCH_STORE_FULL=-5,ETCH_STORE_INCOMPATIBLE=-6};
int etch_store_load(const struct etch_store_io*,struct etch_wave_library*);
int etch_store_save(const struct etch_store_io*,const struct etch_wave_library*);
uint32_t etch_store_crc(const void*,uint32_t);

#define ETCH_STORE_ERASE_CONFIRM 0x45544348u
int etch_store_initialize(const struct etch_store_io*,const struct etch_wave_library*,uint32_t confirmation);

/* Return 1 only when all active wave shapes match their saved slot data. */
int etch_store_patch_waves_ready(const struct etch_wave_library*,const int16_t active[3][128]);

/* V3 preserves 0x97000..0xA1000 unchanged and atomically saves a
 * lossless compact library + 256-step poly pattern when they fit.
 * Out-of-space leaves the previously committed bank untouched. */
#include "native_performance.h"
struct etch_project_settings;
uint8_t etch_store_active_region(void);
int etch_store_load_project(const struct etch_store_io*,struct etch_wave_library*,
                            struct etch_native_pattern*,struct etch_project_settings*);
int etch_store_save_project(const struct etch_store_io*,const struct etch_wave_library*,
                            const struct etch_native_pattern*,const struct etch_project_settings*);
int etch_store_initialize_project(const struct etch_store_io*,const struct etch_wave_library*,
                            const struct etch_native_pattern*,const struct etch_project_settings*,uint32_t);

/* P05 project catalog: four independently addressed, capacity-bounded projects
 * inside one CRC-verified A/B flash snapshot (no new flash partitions).
 * The expanded flash map is still hardware-unverified. */
void etch_store_project_select(uint8_t index);
uint8_t etch_store_project_selected(void);
/* Mark this explicit save as the new global waveform-library master. */
void etch_store_global_wave_save(void);
void etch_store_global_wave_cancel(void);
void etch_store_project_name(const char *name);
int etch_store_projects(const struct etch_store_io *io,uint8_t *occupied,char names[4][12]);
int etch_store_load_project_slot(const struct etch_store_io *io,uint8_t slot,
 struct etch_wave_library *lib,struct etch_native_pattern *pattern,struct etch_project_settings *cfg);

/* Two-phase-confirmed project deletion: copy survivors to the other A/B bank. */
int etch_store_delete_project_slot(const struct etch_store_io*,uint8_t slot,uint32_t confirmation);
