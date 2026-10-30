/* SPDX-License-Identifier: GPL-3.0-only
 * Deterministic VCR-style tracking/head-switch offsets. Splash only,
 * no heap/flash/audio timing, no new framebuffer.
 */
#pragma once
#include <stdint.h>
static inline uint32_t etch_vhs_hash(uint32_t x){
 x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);
}
static inline int8_t etch_vhs_band_shift(uint8_t frame,uint8_t band){
 static const int8_t shift[9][2]={{7,-4},{-5,8},{4,-6},{-8,3},{5,-3},{-3,4},{2,-2},{-2,1},{1,0}};
 return shift[frame%9u][band&1u];
}
static inline uint16_t etch_vhs_dropout_y(uint8_t frame,uint8_t strip){
 return (uint16_t)(79u+(etch_vhs_hash((uint32_t)frame*37u+(uint32_t)strip*97u)%145u));
}
