/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.73: preflight bounds for shared LCD DMA composition memory.
 * No allocation, no flash writes, no audio ISR participation.
 */
#pragma once
#include <stdint.h>
static inline uint8_t etch_ui_canvas_rect_valid(uint32_t top,uint32_t rows,uint32_t capacity){
 return (uint8_t)(rows>0u && rows<=capacity && top<240u && rows<=240u-top);
}
static inline uint8_t etch_ui_panel_top_valid(uint32_t top,uint32_t capacity){
 return (uint8_t)(top<240u && etch_ui_canvas_rect_valid(top,240u-top,capacity));
}
