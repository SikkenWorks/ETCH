/* SPDX-License-Identifier: GPL-3.0-only
 * V1.73 transparent 5x7 glyph blit, no extra framebuffer and no DMA.
 * Returns 0 when the glyph doesn't fit the active canvas. Caller then uses
 * the original conservative renderer, preserving partial clipping semantics.
 */
#pragma once
#include <stdint.h>
static inline uint8_t etch_ui_glyph_fast_565(uint16_t *canvas,uint32_t width,
 uint32_t origin,uint32_t height,uint32_t x,uint32_t y,
 const uint8_t bitmap[7],uint16_t color){
 if(!canvas||!bitmap||width<5u||x>width-5u||origin>240u||height>240u-origin||
    y<origin||y-origin>height||height-(y-origin)<7u)return 0u;
 const uint16_t swapped=(uint16_t)((color<<8)|(color>>8));
 uint16_t *row=canvas+(y-origin)*width+x;
 for(uint32_t r=0;r<7u;r++,row+=width){
  const uint32_t bits=bitmap[r];
  for(uint32_t c=0;c<5u;c++)if(bits&(16u>>c))row[c]=swapped;
 }
 return 1u;
}
