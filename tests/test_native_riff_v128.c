/* SPDX-License-Identifier: GPL-3.0-only. V1.28 motif and rate regression. */
#include "../firmware/fm1-native/native_performance.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct etch_harmony_settings h={0,0,2,0};
 uint32_t base=0x176c;
 int changed=0,normal_notes=0,dense_notes=0,sparse_notes=0;
 for(unsigned step=0;step<16;step++){
  uint8_t old=etch_riff_pick(base,step,60,&h,2);
  assert(etch_riff_pick_variation(base,0,0,50,step,60,&h,2)==old);
  uint8_t mutated=etch_riff_pick_variation(base,0x256a,100,50,step,60,&h,2);
  if(old!=mutated)changed++;
  uint8_t dense=etch_riff_pick_variation(base,0,0,100,step,60,&h,2);
  uint8_t sparse=etch_riff_pick_variation(base,0,0,0,step,60,&h,2);
  if(dense!=255)dense_notes++;
  if(sparse!=255)sparse_notes++;
  if(old!=255)normal_notes++;
  assert(mutated==255||mutated<128);
  if((step&3)==0)assert(mutated==old && dense==old && sparse==old);
  assert(etch_riff_pick_variation(base,0x256a,100,50,step,60,&h,2)==mutated);
 }
 assert(changed>=1);
 assert(dense_notes>=sparse_notes);
 assert(normal_notes>=4);
 puts("PASS V1.28 RIFF original neutral; deterministic motif mutation; anchored beats; density");
}
