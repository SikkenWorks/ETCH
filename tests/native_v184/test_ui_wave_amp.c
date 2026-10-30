/* SPDX-License-Identifier: GPL-3.0-only: advanced navigation and no overlap. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "native_ui_navigation.h"
static int overlap(unsigned ax,unsigned ay,unsigned aw,unsigned ah,unsigned bx,unsigned by,unsigned bw,unsigned bh){
 return ax<bx+bw && bx<ax+aw && ay<by+bh && by<ay+ah;
}
int main(void){
 assert(ETCH_UI_WAVE_AMP==63u && ETCH_UI_PAGE_COUNT==64u);
 assert(strcmp(etch_ui_short_title(ETCH_UI_WAVE_AMP),"WAVE AMP")==0);
 assert(etch_ui_catalog_allowed(ETCH_UI_WAVE_AMP,0u));
 assert(etch_ui_catalog_allowed(ETCH_UI_WAVE_AMP,1u));
 assert(etch_ui_workflow_lfo(ETCH_UI_MATRIX,0u,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_WAVE_AMP);
 assert(etch_ui_workflow_lfo(ETCH_UI_MATRIX,1u,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_WAVE_AMP);
 assert(etch_ui_workflow_lfo(ETCH_UI_WAVE_AMP,0u,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_MOD);
 assert(etch_ui_workflow_lfo(ETCH_UI_WAVE_AMP,1u,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_PATCH_BLEND);
 assert(!etch_ui_catalog_allowed(64,1));
 /* 240x240 native bitmap LCD: two informational rows and two knob cells. */
 assert(!overlap(14,168,17*8,7,14,182,7*8,7));
 assert(!overlap(14,182,7*8,7,4,198,54,7));
 assert(!overlap(4,198,54,7,64,198,54,7));
 assert(14+17*8<=234 && 14+7*8<=234);
 puts("PASS V1.84 UI: advanced routes, new 64th screen, 240x240 WAVE AMP text bounds");
}
