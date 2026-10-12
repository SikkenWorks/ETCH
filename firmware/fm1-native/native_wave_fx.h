/* SPDX-License-Identifier: GPL-3.0-only
 * Offline single-cycle transforms. No persistent settings yet.
 */
#pragma once
#include <stdint.h>
struct etch_wave_fx_params {uint16_t drive,bitcrush,chorus,delay,reverb,fold,smooth,mix;};
void etch_wave_fx_apply(const int16_t src[128],const struct etch_wave_fx_params *params,int16_t out[128]);
