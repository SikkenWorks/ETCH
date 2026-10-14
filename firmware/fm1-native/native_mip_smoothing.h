/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.21: continuous fixed-point mip selection near harmonic boundaries.
 * Pure helper for both target DSP and deterministic host tests.
 */
#pragma once
#include <stdint.h>
/* Transition in the highest 1/8 of each mip region. Output Q15, exactly
 * zero below the transition and 32768 at the existing boundary. All
 * arithmetic is unsigned integer, with no heap or floating-point DSP. */
static inline uint32_t etch_mip_boundary_alpha(uint32_t inc,uint32_t boundary){
 uint32_t start=boundary-(boundary>>3);
 if(inc<=start)return 0u;
 if(inc>=boundary)return 32768u;
 uint32_t width=boundary-start;
 return (uint32_t)(((uint64_t)(inc-start)*32768u)/width);
}
