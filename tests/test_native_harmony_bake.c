/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/native_performance.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct etch_native_pattern p,orig;
int main(void){
 struct etch_harmony_settings h={0,0,2,2};
 etch_pattern_clear(&p);
 assert(etch_seq_put(&p,0,60,120,3));
 assert(etch_seq_put(&p,0,64,100,2));
 assert(etch_seq_put(&p,1,62,95,4));
 p.steps[1].lock_mask=3;p.steps[1].locks[0]=1234;p.steps[1].locks[1]=25000;
 assert(etch_seq_hold_toggle(&p,2,62)==1);
 memcpy(&orig,&p,sizeof p);
 assert(etch_seq_bake_harmony(&p,16,0,1,&h,0)==2);
 assert(!memcmp(&orig,&p,sizeof p)); /* preview must not write */
 assert(etch_seq_bake_harmony(&p,16,0,0,&h,1)==1);
 assert(p.steps[0].count==4);
 assert(p.steps[0].notes[0].note==60&&p.steps[0].notes[1].note==64&&p.steps[0].notes[2].note==67&&p.steps[0].notes[3].note==71);
 assert(p.steps[0].notes[0].length==3);
 assert(p.steps[1].count==1&&p.steps[1].notes[0].note==62);
 assert(etch_seq_bake_harmony(&p,16,0,1,&h,1)==1);
 assert(p.steps[1].count==4&&p.steps[1].notes[0].note==62);
 assert(p.steps[1].lock_mask==3&&p.steps[1].locks[0]==1234&&p.steps[1].locks[1]==25000);
 assert(p.steps[2].hold_count==1&&p.steps[2].holds[0]==62);
 assert(etch_seq_bake_harmony(&p,16,0,1,&h,0)==0); /* idempotent */
 /* Keylock with chord OFF, collapsing duplicate pitches with maximal expression. */
 etch_pattern_clear(&p);h.chord=0;h.keylock=2;
 assert(etch_seq_put(&p,0,60,80,1));assert(etch_seq_put(&p,0,61,110,4));
 assert(etch_seq_bake_harmony(&p,8,0,1,&h,0)==1);
 assert(etch_seq_bake_harmony(&p,8,0,1,&h,1)==1);
 assert(p.steps[0].count==1&&p.steps[0].notes[0].note==60);
 assert(p.steps[0].notes[0].velocity==110&&p.steps[0].notes[0].length==4);
 memcpy(&orig,&p,sizeof p);
 h.mode=20;assert(etch_seq_bake_harmony(&p,8,0,1,&h,1)==-1);
 assert(!memcmp(&orig,&p,sizeof p));
 h.mode=0;assert(etch_seq_bake_harmony(&p,257,0,1,&h,1)==-1);
 assert(etch_seq_bake_harmony(&p,8,8,0,&h,1)==-1);
 assert(etch_seq_bake_harmony(&p,8,0,2,&h,1)==-1);
 puts("PASS harmony preview, step/whole pattern BAKE, chord length, lock/HOLD preservation, dedupe, invalid rollback");
}
