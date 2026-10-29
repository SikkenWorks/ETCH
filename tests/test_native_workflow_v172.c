#include "../firmware/fm1-native/native_ui_navigation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 assert(ETCH_UI_PAGE_COUNT==62u);
 for(unsigned start=0;start<62;start++){
  uint8_t page=(uint8_t)start;
  assert(page==etch_ui_catalog_turn(0,(int32_t)start));
  assert(etch_ui_catalog_turn(page,62)==page);
  assert(etch_ui_catalog_turn(page,-62)==page);
  assert(etch_ui_catalog_turn(page,1)==(start+1u)%62u);
  assert(etch_ui_catalog_turn(page,-1)==(start+61u)%62u);
  assert(etch_ui_catalog_allowed(page,1u));
  assert(strlen(etch_ui_title(page))>0u);
  if(etch_ui_mono_only(page))assert(!etch_ui_catalog_allowed(page,0u));
  else assert(etch_ui_catalog_allowed(page,0u));
 }
 assert(etch_ui_catalog_needs_draw(ETCH_UI_TOOLS));
 assert(etch_ui_catalog_needs_draw(ETCH_UI_WFX1));
 assert(etch_ui_catalog_needs_draw(ETCH_UI_WFX2));
 assert(etch_ui_catalog_needs_draw(ETCH_UI_SCULPT));
 assert(!etch_ui_catalog_needs_draw(ETCH_UI_PLAY));
 assert(!etch_ui_catalog_allowed(62,1));
 assert(etch_ui_home_workflow_turn(ETCH_UI_PLAY,1,0)==ETCH_UI_MORPH);
 for(unsigned i=0;i<8;i++)assert(etch_ui_system_tab_workflow(i,1)==(i+1u)%8u);
 puts("PASS V1.72: 62/62 catalog routes, wraparound, mono-only rules, draw modes, SIMPLE retained");
}
