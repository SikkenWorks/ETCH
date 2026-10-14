/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/wave_editor.h"
#include "../firmware/fm1-native/native_ui_navigation.h"
static struct etch_wave_editor e;
static struct etch_native_bank bank;
int main(void){
 etch_editor_init(&e);uint16_t before=etch_editor_spectral_level(&e,3);
 int16_t untouched[128];memcpy(untouched,e.points[1],sizeof(untouched));
 uint32_t original=e.revision;
 e.wave=0;etch_editor_spectral_partial(&e,3,4);
 assert(e.revision>original&&e.dirty);
 assert(etch_editor_spectral_level(&e,3)>before);
 assert(!memcmp(untouched,e.points[1],sizeof(untouched)));
 etch_editor_undo(&e);assert(etch_editor_spectral_level(&e,3)==before);
 etch_editor_redo(&e);assert(etch_editor_spectral_level(&e,3)>before);
 etch_editor_spectral_tilt(&e,6);assert(e.revision>=original+2);
 etch_editor_compile(&e,&bank);
 for(int w=0;w<3;w++)for(int mip=0;mip<6;mip++)for(int n=0;n<512;n++){
  assert(bank.table[w][mip][n]>=-29000 && bank.table[w][mip][n]<=29000);
 }
 assert(etch_ui_title(ETCH_UI_SPECTRAL)!=0);
 assert(etch_ui_is_home(ETCH_UI_SPECTRAL)==0);
 assert(etch_editor_spectral_brightness(&e)<=100);
 puts("PASS V1.24 spectral harmonic editing, undo/redo, isolation, bank limits, UI page");
}
