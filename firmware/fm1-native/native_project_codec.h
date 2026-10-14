/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V10 backward-readable compact snapshot codec. Pure, bounded, host-testable code.
 * It does not access flash and never truncates an oversized project.
 */
#pragma once
#include <stdint.h>
#include "wave_store.h"
#include "native_performance.h"
#include "native_wave_fx.h"
#include "native_pattern_bank.h"
#include "native_fx.h"
#include "native_motion.h"
#include "native_evolution.h"
#include "native_organic.h"
#include "native_living.h"
#include "native_mod_matrix.h"
#define ETCH_PROJECT_MAX_BYTES ((ETCH_P04_EXPANDED ? ETCH_STORE_EXT_BANK_BYTES : ETCH_STORE_SPARE_ENABLED ? ETCH_STORE_SPARE_BANK_BYTES : ETCH_STORE_BANK_BYTES)-256u)
/* PJT26: independently saved, non-audio-IRQ state for each named preset. */
struct etch_preset_dsp {
 struct etch_organic_settings organic;
 struct etch_living_settings living;
 struct etch_mod_matrix matrix;
 struct etch_evolution_settings evolution;
 uint8_t spectral_wave,spectral_part;
 uint8_t blend_a,blend_b,blend_pct,blend_on;
 uint8_t spectral_gain,spectral_tilt; /* PJT27: independent control positions, centre = 50 */
};
struct etch_preset_dsp_bank {struct etch_preset_dsp slot[ETCH_PATCH_SLOTS];};
_Static_assert(sizeof(struct etch_preset_dsp)==32u,"PJT27 record size must stay exact; PJT26 prefix is 30 bytes");
struct etch_project_settings {
 uint16_t seq_length;
 uint8_t seq_rate,seq_gate,seq_swing,tempo,clock_follow;
 struct etch_harmony_settings harmony;
 struct etch_wave_fx_params wave_fx[3];
 struct etch_wave_fx_params slot_fx[ETCH_WAVE_SLOTS];
 uint32_t patch_fx_valid;
 struct etch_wave_fx_params patch_fx[ETCH_PATCH_SLOTS][3];
 struct etch_pattern_bank patterns;
 struct etch_fx_settings global_fx;
 /* PJT13: presets own their sound, not just references to mutable wave slots. */
 uint32_t preset_valid;
 struct etch_fx_settings preset_fx[ETCH_PATCH_SLOTS];
 int16_t preset_wave[ETCH_PATCH_SLOTS][3][128];
 uint8_t pattern_rate[ETCH_PATCH_SLOTS],pattern_gate[ETCH_PATCH_SLOTS],pattern_swing[ETCH_PATCH_SLOTS];
 struct etch_arp_program arp_programs[ETCH_PATCH_SLOTS]; /* V7+ custom ARP, sparse encoding */
 struct etch_motion_clip motion; /* PJT18: per-project motion loop */
 struct etch_evolution_settings evolution; /* PJT19: per-project Wave Evolution */
 struct etch_organic_settings organic; /* PJT22: four project-scoped Organic controls */
 struct etch_living_settings living; /* PJT23: four project-scoped Living controls */
 struct etch_mod_matrix matrix; /* PJT24: four project-scoped modulation routes */

 uint8_t mono_mode; /* 0=POLY, 1=MONO; saved per project, V12+ */
 uint8_t source_version; /* read-only runtime metadata; not encoded */
};
/* PJT25 per-preset RIFF extras are allocated separately from the full DSP
 * project snapshot: the FM1 audio pool has no space for more project bytes. */
struct etch_riff2_bank {
 uint16_t riff_variants[ETCH_PATCH_SLOTS];
 uint8_t riff_mutations[ETCH_PATCH_SLOTS],riff_densities[ETCH_PATCH_SLOTS];
};
void etch_project_bind_preset_dsp(struct etch_project_settings *project,struct etch_preset_dsp_bank *bank);
void etch_project_copy_preset_dsp(struct etch_project_settings *destination,const struct etch_project_settings *source);
void etch_project_bind_riff2(struct etch_project_settings *project,struct etch_riff2_bank *bank);
/* Conservative worst-case PJT10 serialization upper bound for the
 * CURRENT feature model.  Patterns are bounded by the separate 16 KiB
 * pattern archive, so this DOES NOT mean 32 completely filled 256-step
 * patterns fit in that archive. Uncompressed 16-bit waves are accounted for,
 * as are all 256 occupied steps with 8 triggers, 8 HOLD links and 6 locks,
 * all 32 patches and custom ARPs, and all wave/patch FX slots.
 * This is a source-level capacity proof for the current codec, not evidence
 * that the physical flash map is safe. */
#define ETCH_P04_CURRENT_WORST_BYTES ( \
  4u+4u+8u + sizeof(((struct etch_wave_library*)0)->active) + \
  sizeof(((struct etch_wave_library*)0)->wave_names) + 16u + \
  ETCH_WAVE_SLOTS*128u*2u + 4u + \
  ETCH_PATCH_SLOTS*sizeof(struct etch_native_patch) + \
  sizeof(((struct etch_wave_library*)0)->sequence) + \
  7u+sizeof(struct etch_harmony_settings) + \
  3u*sizeof(struct etch_wave_fx_params) + 8u + \
  ETCH_WAVE_SLOTS*sizeof(struct etch_wave_fx_params) + 4u + \
  ETCH_PATCH_SLOTS*3u*sizeof(struct etch_wave_fx_params) + \
  2u + ETCH_SEQ_STEPS*(4u+ETCH_SEQ_NOTES*3u+ETCH_SEQ_NOTES*2u+ETCH_SEQ_LOCKS*2u) + \
  2u+ETCH_PATCH_SLOTS*2u+1u+2u+ETCH_PATTERN_ARCHIVE_BYTES + \
  ETCH_PATCH_SLOTS*3u + 13u + 1u + \
  ETCH_PATCH_SLOTS*(2u+ETCH_ARP_PROGRAM_STEPS) + 1024u )
#if ETCH_P04_EXPANDED
_Static_assert(ETCH_P04_CURRENT_WORST_BYTES <= ETCH_PROJECT_MAX_BYTES,
               "current PJT10 content model must fit one expanded flash bank");
#endif
/* 0 = success, -1 = invalid/corrupt, -2 = no space. */
/* Optional V21 companion buffer: caller owns the data, and the codec only
 * reads/writes it for this exact project object (not temporary validation). */
void etch_project_bind_riffs(struct etch_project_settings *project,uint16_t *seeds);
int etch_project_encode(const struct etch_wave_library *lib,
 const struct etch_native_pattern *pat,const struct etch_project_settings *settings,
 uint8_t *dst,uint32_t cap,uint32_t *used);
int etch_project_decode(const uint8_t *src,uint32_t len,
 struct etch_wave_library *lib,struct etch_native_pattern *pat,
 struct etch_project_settings *settings);
void etch_project_legacy(const struct etch_wave_library *lib,
 struct etch_native_pattern *pat,struct etch_project_settings *settings);

/* Exact encoded size, even when it exceeds a single on-device bank.
 * No flash access, no output buffer, zero allocations. */
int etch_project_measure(const struct etch_wave_library*,const struct etch_native_pattern*,
 const struct etch_project_settings*,uint32_t *required);
