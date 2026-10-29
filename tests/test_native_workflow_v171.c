/* V1.71 workflow routing and 240x240 spatial text constraints.
 * Software-only, not a physical LCD rendering validation. */
#include "../firmware/fm1-native/native_ui_navigation.h"
#include "../firmware/fm1-native/native_ui_refresh_v170.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void test_titles(void){
 /* V1.71 top/title region: 88..194 = 106 px -> 13 8px glyphs.
  * Legacy auxiliary page status region 12..108 = 96 px -> 12 glyphs.
  * Longer names are bounded by label_box at render time. */
 for(unsigned p=0;p<ETCH_UI_PAGE_COUNT;p++){
  const char *title=etch_ui_short_title((uint8_t)p);
  assert(title && title[0]);
  assert(strlen(title)<=13u);
  assert(strlen(etch_ui_title((uint8_t)p))>0u);
 }
 assert(strcmp(etch_ui_short_title(ETCH_UI_PROJECT_SAVE),"PROJ SAVE")==0);
 assert(strcmp(etch_ui_short_title(ETCH_UI_FX_TAPE),"TAPE FX")==0);
 assert(ETCH_UI_PAGE_COUNT==62);
}
static void test_footer_numeric(void){
 char value[11];
 uint32_t samples[]={0,1,7,10,50,99,100,20000,999999,1234567,4294967295u};
 for(unsigned width=1;width<=7;width++){
  for(unsigned t=0;t<sizeof(samples)/sizeof(samples[0]);t++){
   etch_ui_cell_number(samples[t],value,(uint8_t)width);
   assert(strlen(value)<=width);
   assert(strlen(value)>0u);
   if(samples[t]<10u){assert(value[0]==(char)('0'+samples[t]));}
   if(strlen(value)==width && value[width-1]=='+')assert(samples[t]>9u);
  }
 }
 /* Footer four cells are guaranteed disjoint with max-width 53px and 7px gaps. */
 for(unsigned k=0;k<4;k++){
  const unsigned x=4+k*60u;
  const unsigned right=etch_ui_footer_right((uint8_t)k);
  assert(x<right&&right<=240u);
  assert((right-x)/8u<=7u);
  if(k<3u)assert(right < 4+(k+1u)*60u);
  else assert(right==240u);
 }
}
static void test_workflow(void){
 uint8_t page=ETCH_UI_PLAY;
 const uint8_t simple[]={ETCH_UI_MORPH,ETCH_UI_FILTER,ETCH_UI_FX,ETCH_UI_LIBRARY,ETCH_UI_PLAY};
 for(unsigned i=0;i<15;i++){
  page=etch_ui_home_workflow_turn(page,1,ETCH_WORKFLOW_SIMPLE);
  assert(page==simple[i%5]);
  assert(etch_ui_home_workflow_page(page,ETCH_WORKFLOW_SIMPLE));
 }
 for(unsigned i=0;i<15;i++)page=etch_ui_home_workflow_turn(page,-1,ETCH_WORKFLOW_SIMPLE);
 assert(page==ETCH_UI_PLAY);
 assert(!etch_ui_home_workflow_page(ETCH_UI_SEQ,ETCH_WORKFLOW_SIMPLE));
 assert(!etch_ui_home_workflow_page(ETCH_UI_FX,ETCH_WORKFLOW_ADVANCED));
 assert(etch_ui_home_workflow_turn(ETCH_UI_PLAY,1,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_MORPH);
 assert(etch_ui_home_workflow_turn(ETCH_UI_FX,1,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_FX);
 assert(etch_ui_workflow_fx(ETCH_UI_PLAY,ETCH_WORKFLOW_SIMPLE)==ETCH_UI_FX);
 assert(etch_ui_workflow_fx(ETCH_UI_FX_DRIVE,ETCH_WORKFLOW_SIMPLE)==ETCH_UI_FX);
 assert(etch_ui_workflow_fx(ETCH_UI_FX,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_FX_DRIVE);
 assert(etch_ui_workflow_fx(ETCH_UI_FX_DRIVE,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_PREFX_A);
 assert(etch_ui_workflow_lfo(ETCH_UI_MATRIX,1,ETCH_WORKFLOW_SIMPLE)==ETCH_UI_MOD);
 assert(etch_ui_workflow_lfo(ETCH_UI_MATRIX,1,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_PATCH_BLEND);
 assert(etch_ui_workflow_seq(ETCH_UI_SEQ,ETCH_WORKFLOW_SIMPLE)==ETCH_UI_NOTES);
 assert(etch_ui_workflow_seq(ETCH_UI_NOTES,ETCH_WORKFLOW_SIMPLE)==ETCH_UI_SEQ);
 assert(etch_ui_workflow_seq(ETCH_UI_NOTES,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_HARM);
 assert(etch_ui_workflow_arp(ETCH_UI_ARP,ETCH_WORKFLOW_SIMPLE)==ETCH_UI_ARP);
 assert(etch_ui_workflow_arp(ETCH_UI_ARP,ETCH_WORKFLOW_ADVANCED)==ETCH_UI_ARP_EDIT);
 assert(etch_ui_workflow_glo(ETCH_UI_PLAY,ETCH_WORKFLOW_SIMPLE,0)==ETCH_UI_SYSTEM);
 assert(etch_ui_workflow_glo(ETCH_UI_SYSTEM,ETCH_WORKFLOW_SIMPLE,0)==ETCH_UI_PLAY);
 assert(etch_ui_workflow_glo(ETCH_UI_PLAY,ETCH_WORKFLOW_ADVANCED,0)==ETCH_UI_GUIDE);
 for(unsigned i=0;i<8;i++){
  assert(etch_ui_system_tab_workflow(i,1)==(i+1u)%8u);
  assert(etch_ui_system_tab_workflow(i,-1)==(i+7u)%8u);
 }
 assert(etch_ui_system_tab_workflow(6,1)==7u);
 assert(etch_ui_system_tab_workflow(7,1)==0u);
 assert(etch_ui_workflow_change(ETCH_WORKFLOW_SIMPLE,1)==ETCH_WORKFLOW_ADVANCED);
 assert(etch_ui_workflow_change(ETCH_WORKFLOW_ADVANCED,-1)==ETCH_WORKFLOW_SIMPLE);
 assert(etch_ui_workflow_change(ETCH_WORKFLOW_ADVANCED,0)==ETCH_WORKFLOW_ADVANCED);
}
int main(void){
 test_titles();test_footer_numeric();test_workflow();
 assert(etch_ui_refresh_interval(32,1,4)==150);
 puts("PASS V1.71: 62 page titles, fixed 4-column numeric bounds, SIMPLE/ADVANCED routing, 8 system tabs");
 return 0;
}
