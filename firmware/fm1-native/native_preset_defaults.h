/* SPDX-License-Identifier: GPL-3.0-only
 * Factory and legacy presets without explicit independent FX snapshots must
 * never inherit the project's Wave C sculpt or Wave C FX modulation settings.
 * User-saved presets with snapshots are deliberately left unchanged.
 */
#pragma once
#include "native_fx.h"
#include "native_wave_fx.h"
static inline void etch_preset_legacy_c_fx_off(struct etch_wave_fx_params *wave_c,
                                               struct etch_fx_settings *fx){
 if(wave_c){struct etch_wave_fx_params neutral={0};neutral.mix=32767u;*wave_c=neutral;}
 if(fx)fx->c_fx=0u;
}
