/* ETCH V1.79: intro timeline, workflow choice, and dirty-only modal rendering. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_boot_visual_v179.h"
#include "../firmware/fm1-native/native_storage_prompt_paint.h"
#define SAVE (1u<<9)
int main(void){
 assert(ETCH_BOOT_GLITCH_FRAMES==9);
 assert(ETCH_BOOT_BRAND_HOLD_MS==1000);
 assert(etch_boot_mode_pick(0,0,0)==0);
 assert(etch_boot_mode_pick(0,1,0)==1);
 assert(etch_boot_mode_pick(1,-1,0)==0);
 for(unsigned f=0;f<ETCH_BOOT_GLITCH_FRAMES;f++){
  int8_t s=etch_boot_rgb_shift((uint8_t)f);
  assert(s>=-3 && s<=3);
  assert(etch_boot_rgb_shift((uint8_t)(f+ETCH_BOOT_GLITCH_FRAMES))==s);
 }
 struct etch_storage_prompt_paint cache={0};
 assert(etch_storage_prompt_paint_needed(&cache,1,0,0,0,0,1));
 for(unsigned i=0;i<2000;i++)
  assert(!etch_storage_prompt_paint_needed(&cache,1,0,0,0,0,0));
 assert(etch_storage_prompt_paint_needed(&cache,1,1,0,0,0,0));
 assert(!etch_storage_prompt_paint_needed(&cache,1,1,0,0,0,0));
 assert(etch_storage_prompt_paint_needed(&cache,2,1,0,0,0,0));
 assert(etch_storage_prompt_paint_needed(&cache,2,1,0,0,1,0));
 struct etch_storage_boot_prompt p={0};p.phase=ETCH_STORAGE_PROMPT_CONFIRM;
 assert(!etch_storage_prompt_progress(&p,SAVE,6000));
 p.save_released=1;p.save_held_at=1000;
 uint8_t prior=0;
 for(uint32_t t=1000;t<=4000;t+=20){
  uint8_t next=etch_storage_prompt_progress(&p,SAVE,t);
  assert(next<=ETCH_STORAGE_PROGRESS_STEPS && next>=prior);
  prior=next;
 }
 assert(prior==ETCH_STORAGE_PROGRESS_STEPS);
 assert(etch_storage_prompt_progress(&p,0,4000)==0);
 assert(etch_storage_prompt_paint_needed(&cache,2,1,0,0,0,1));
 assert(!etch_storage_prompt_paint_needed(&cache,2,1,0,0,0,0));
 assert(etch_storage_prompt_paint_needed(&cache,2,1,0,0,0,1));
 puts("PASS V1.79: RGB glitch timeline; mode input; storage modal zero redundant draws; hold progression");
 return 0;
}
