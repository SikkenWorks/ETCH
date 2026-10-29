/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "wave_store.h"
/* Pure RAM initializer; NEVER erases/programs flash. */
void etch_factory_seed(struct etch_wave_library *library);

/* Vintage string-machine inspired starting point, not an exact Solina model.
 * Wave generator is deterministic and uses existing ROM tables; no new heap. */
void etch_factory_strings_patch(struct etch_native_patch *patch);
void etch_factory_strings_wave(uint8_t wave,int16_t points[128]);
