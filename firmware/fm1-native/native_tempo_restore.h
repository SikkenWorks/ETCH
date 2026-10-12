/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V0.93: project BPM is authoritative on project reopen/boot.
 * A live patch change continues to load the saved patch's BPM.
 */
#pragma once
#include <stdint.h>
static inline uint8_t etch_saved_project_bpm(uint8_t saved,uint8_t fallback){
 if(saved>=30u&&saved<=240u)return saved;
 if(fallback>=30u&&fallback<=240u)return fallback;
 return 120u;
}
