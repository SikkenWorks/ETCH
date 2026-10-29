/* SPDX-License-Identifier: GPL-3.0-only
 * V1.73 output equivalence, guard-canary and 240 x 240 canvas preflight tests.
 * Host tests; not a substitute for physical LCD screenshots.
 */
#include "../firmware/fm1-native/native_ui_glyph_fast_v173.h"
#include "../firmware/fm1-native/native_ui_bounds_v173.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define WIDTH 240u
#define HEIGHT 173u
static uint16_t fast[WIDTH*HEIGHT+8u],slow[WIDTH*HEIGHT+8u];
static uint32_t randstate=0x17417317u;
static uint32_t rnd(void){randstate^=randstate<<13;randstate^=randstate>>17;randstate^=randstate<<5;return randstate;}
static void old_paint(uint16_t *dst,uint32_t origin,uint32_t x,uint32_t y,const uint8_t glyph[7],uint16_t color){
 const uint16_t swapped=(uint16_t)((color<<8)|(color>>8));
 for(uint32_t row=0;row<7;row++)for(uint32_t col=0;col<5;col++)if(glyph[row]&(16u>>col))
  dst[(y-origin+row)*WIDTH+x+col]=swapped;
}
int main(void){
 assert(etch_ui_panel_top_valid(67u,HEIGHT));
 assert(!etch_ui_panel_top_valid(66u,HEIGHT));
 assert(!etch_ui_panel_top_valid(240u,HEIGHT));
 assert(etch_ui_canvas_rect_valid(0u,66u,HEIGHT));
 assert(etch_ui_canvas_rect_valid(67u,173u,HEIGHT));
 assert(!etch_ui_canvas_rect_valid(67u,174u,HEIGHT));
 assert(!etch_ui_canvas_rect_valid(239u,2u,HEIGHT));
 assert(!etch_ui_canvas_rect_valid(240u,1u,HEIGHT));
 assert(!etch_ui_canvas_rect_valid(0u,0u,HEIGHT));
 for(uint32_t iter=0;iter<8192u;iter++){
  const uint32_t origin=(rnd()%68u);
  const uint32_t height=HEIGHT;
  const uint32_t x=rnd()%(WIDTH-4u);
  const uint32_t y=origin+(rnd()%(height-6u));
  uint8_t bitmap[7];for(uint32_t i=0;i<7u;i++)bitmap[i]=(uint8_t)(rnd()&31u);
  const uint16_t color=(uint16_t)rnd();
  for(uint32_t i=0;i<WIDTH*HEIGHT+8u;i++){fast[i]=slow[i]=(uint16_t)rnd();}
  assert(etch_ui_glyph_fast_565(fast,WIDTH,origin,height,x,y,bitmap,color));
  old_paint(slow,origin,x,y,bitmap,color);
  assert(memcmp(slow,fast,sizeof fast)==0);
  assert(!etch_ui_glyph_fast_565(fast,WIDTH,origin,height,WIDTH-4u,y,bitmap,color));
  assert(!etch_ui_glyph_fast_565(fast,WIDTH,origin,height,x,origin+height-6u,bitmap,color));
  assert(!etch_ui_glyph_fast_565(fast,WIDTH,origin,height,x,origin-1u,bitmap,color));
 }
 puts("PASS 8192 random RGB565 glyph exact comparisons, guard canaries and canvas bounds");
}
