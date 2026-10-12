/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/native_performance.h"
#include <assert.h>
#include <stdio.h>
static const uint16_t ionian=(1u<<0)|(1u<<2)|(1u<<4)|(1u<<5)|(1u<<7)|(1u<<9)|(1u<<11);
int main(void){
 struct etch_harmony_guide g;struct etch_harmony_settings h={0,0,2,2};
 assert(!etch_harmony_guide_build(&h,60,&g));
 assert(g.scale_mask==ionian&&g.count==4&&g.notes[0]==60&&g.notes[3]==71);
 assert((g.chord_mask&(1u<<0))&&(g.chord_mask&(1u<<11)));
 for(uint8_t m=0;m<9;m++){
  h.mode=m;assert(!etch_harmony_guide_build(&h,62,&g));
  uint8_t bits=0;for(uint8_t k=0;k<12;k++)if(g.scale_mask&(1u<<k))bits++;
  assert(bits==7);
  for(uint8_t k=0;k<g.count;k++)assert(g.scale_mask&(1u<<(g.notes[k]%12)));
 }
 h.mode=0;h.root=5;assert(!etch_harmony_guide_build(&h,67,&g));
 assert(g.scale_mask==(uint16_t)(((ionian<<5)|(ionian>>(12-5)))&4095u));
 h.mode=9;assert(etch_harmony_guide_build(&h,60,&g)==-1);
 h.mode=0;assert(etch_harmony_guide_build(&h,128,&g)==-1);
 puts("PASS native GUIDE nine scales, pitch-class sets, chord previews, rotating root, invalid input");
}
