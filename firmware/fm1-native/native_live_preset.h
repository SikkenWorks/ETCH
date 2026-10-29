/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.77 RAM-only preset interchange. Little-endian native pi32v2 ABI.
 * Version 1 is defined by sizeof/field layout and may not be silently changed.
 * Use host SHA-256 plus device CRC32; never treat CRC as authentication.
 */
#pragma once
#include <stdint.h>
#include "native_project_codec.h"
#define ETCH_LIVE_MAGIC 0x48435445u /* ETCH in little endian */
struct etch_live_preset {
 uint32_t magic;
 uint16_t version,bytes;
 struct etch_native_patch patch;
 int16_t points[3][128];
 struct etch_wave_fx_params wave_fx[3];
 struct etch_fx_settings global_fx;
 struct etch_preset_dsp dsp;
 struct etch_harmony_settings harmony;
 uint8_t mono_mode,arp_rate,arp_octaves,seq_rate,seq_gate,seq_swing;
};
static inline uint8_t etch_live_validate(const struct etch_live_preset *v){
 if(!v||v->magic!=ETCH_LIVE_MAGIC||v->version!=1u||v->bytes!=sizeof(*v)||
    v->patch.slot_a>=64u||v->patch.slot_b>=64u||v->patch.slot_c>=64u||
    v->patch.mod_mode>2u||v->patch.mod_target>1u||v->patch.mod_ratio>=12u||
    v->patch.filter_type>=4u||v->patch.morph>32767u||v->patch.depth>32767u||
    v->patch.sustain>32767u||v->patch.cutoff>32767u||v->patch.resonance>32767u||
    v->patch.drive>32767u||v->patch.chorus>32767u||v->patch.delay>32767u||v->patch.reverb>32767u||
    v->dsp.velocity_curve>2u||v->dsp.spectral_wave>2u||v->dsp.spectral_part>32u||
    v->dsp.spectral_gain>100u||v->dsp.spectral_tilt>100u||
    v->dsp.blend_a>=32u||v->dsp.blend_b>=32u||v->dsp.blend_pct>100u||v->dsp.blend_on>1u||
    !etch_organic_valid(&v->dsp.organic)||!etch_living_valid(&v->dsp.living)||
    v->mono_mode>1u||v->arp_octaves>4u||v->seq_gate>100u||v->seq_swing>75u||
    v->harmony.root>=12u||v->harmony.mode>=9u||v->harmony.keylock>2u||
    v->global_fx.routing>1u)return 0u;
 return 1u;
}
_Static_assert(sizeof(struct etch_live_preset)<4096u,"live preset must fit the bounded MIDI export");
