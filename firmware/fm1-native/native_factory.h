/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "wave_store.h"
/* Pure RAM initializer; NEVER erases/programs flash. */
void etch_factory_seed(struct etch_wave_library *library);
