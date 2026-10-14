#include <assert.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_ui_navigation.h"
int main(void){
const uint8_t poly[]={ETCH_UI_MOD,ETCH_UI_C_FX,ETCH_UI_MATRIX};
const uint8_t mono[]={ETCH_UI_MOD,ETCH_UI_C_FX,ETCH_UI_EVOLUTION,ETCH_UI_ORGANIC,ETCH_UI_SPECTRAL,ETCH_UI_LIVING,ETCH_UI_MATRIX,ETCH_UI_PATCH_BLEND};
uint8_t page=ETCH_UI_MOD;
for(unsigned i=0;i<3;i++){assert(page==poly[i]);assert(etch_ui_lfo_visible(page,0));page=etch_ui_lfo_next(page,0);}assert(page==ETCH_UI_MOD);
page=ETCH_UI_MOD;for(unsigned i=0;i<8;i++){assert(page==mono[i]);assert(etch_ui_lfo_visible(page,1));page=etch_ui_lfo_next(page,1);}assert(page==ETCH_UI_MOD);
for(unsigned i=0;i<5;i++){uint8_t p=(uint8_t[]){ETCH_UI_EVOLUTION,ETCH_UI_ORGANIC,ETCH_UI_SPECTRAL,ETCH_UI_LIVING,ETCH_UI_PATCH_BLEND}[i];assert(etch_ui_mono_only(p));assert(!etch_ui_lfo_visible(p,0));assert(etch_ui_lfo_next(p,0)==ETCH_UI_MOD);assert(etch_ui_lfo_visible(p,1));}
puts("PASS V1.35 LFO visibility and voice-mode transitions");}
