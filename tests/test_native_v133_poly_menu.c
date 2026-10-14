/* SPDX-License-Identifier: GPL-3.0-only
 * V1.33: menu visibility in both modes, transition out of hidden pages.
 */
#include <assert.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_ui_navigation.h"
int main(void){
 const uint8_t poly[]={ETCH_UI_MOD,ETCH_UI_C_FX,ETCH_UI_MATRIX};
 const uint8_t mono[]={ETCH_UI_MOD,ETCH_UI_C_FX,ETCH_UI_EVOLUTION,
                       ETCH_UI_ORGANIC,ETCH_UI_SPECTRAL,ETCH_UI_LIVING,
                       ETCH_UI_MATRIX,ETCH_UI_PATCH_BLEND};
 uint8_t p=ETCH_UI_MOD;
 for(unsigned i=0;i<3;i++){
  assert(p==poly[i]); assert(etch_ui_lfo_visible(p,0));
  p=etch_ui_lfo_next(p,0);
 }
 assert(p==ETCH_UI_MOD);
 p=ETCH_UI_MOD;
 for(unsigned i=0;i<8;i++){
  assert(p==mono[i]);assert(etch_ui_lfo_visible(p,1));
  p=etch_ui_lfo_next(p,1);
 }
 assert(p==ETCH_UI_MOD);
 assert(!etch_ui_lfo_visible(ETCH_UI_EVOLUTION,0));
 assert(!etch_ui_lfo_visible(ETCH_UI_ORGANIC,0));
 assert(!etch_ui_lfo_visible(ETCH_UI_SPECTRAL,0));
 assert(!etch_ui_lfo_visible(ETCH_UI_LIVING,0));
 assert(!etch_ui_lfo_visible(ETCH_UI_PATCH_BLEND,0));
 assert(etch_ui_lfo_visible(ETCH_UI_SPECTRAL,1));
 assert(etch_ui_lfo_visible(ETCH_UI_LIVING,1));
 assert(etch_ui_lfo_visible(ETCH_UI_PATCH_BLEND,1));
 /* Every mono-only page must recover into a visible poly page. */
 for(unsigned i=0;i<5;i++){
  uint8_t page=(uint8_t[]){ETCH_UI_EVOLUTION,ETCH_UI_ORGANIC,ETCH_UI_SPECTRAL,ETCH_UI_LIVING,ETCH_UI_PATCH_BLEND}[i];
  assert(!etch_ui_lfo_visible(page,0));
  assert(etch_ui_lfo_next(page,0)==ETCH_UI_MOD);
 }
 puts("PASS V1.35 MONO/POLY LFO navigation and mono-only visibility");
}
