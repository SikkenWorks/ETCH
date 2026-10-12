/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/native_performance.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct etch_native_pattern p;
static struct etch_step_layers layer;
int main(void){
 etch_pattern_clear(&p);
 assert(etch_seq_put(&p,15,60,100,3));
 assert(etch_seq_put(&p,0,60,127,3));
 assert(etch_seq_put(&p,1,62,90,1));
 etch_seq_layers(&p,16,0,&layer);
 assert(layer.trigger_count==1&&layer.hold_voices==1&&layer.hold_pitches==1&&layer.overlap_pitches==1);
 assert((layer.trigger_bits[7]&16)&&(layer.hold_bits[7]&16));
 etch_seq_layers(&p,16,1,&layer);
 assert(layer.trigger_count==1&&layer.hold_voices==2&&layer.hold_pitches==1&&layer.overlap_pitches==0);
 etch_seq_layers(&p,16,2,&layer);
 assert(layer.trigger_count==0&&layer.hold_voices==1&&layer.hold_pitches==1);
 assert(etch_seq_toggle(&p,0,60,127,1)==2);
 etch_seq_layers(&p,16,0,&layer);
 assert(layer.trigger_count==0&&layer.hold_voices==1&&layer.overlap_pitches==0);
 assert(etch_seq_toggle(&p,0,60,127,1)==1);
 etch_seq_layers(&p,16,0,&layer);
 assert(layer.overlap_pitches==1);
 etch_seq_layers(&p,8,0,&layer);
 assert(layer.trigger_count==1&&layer.hold_voices==0);
 etch_seq_layers(&p,0,0,&layer);
 assert(!layer.trigger_count&&!layer.hold_voices);
 /* Authored HOLD is independent of derived note length, can overlap new triggers. */
 etch_pattern_clear(&p);assert(etch_seq_put(&p,0,60,110,1));
 assert(etch_seq_hold_toggle(&p,1,60)==1&&etch_seq_hold_toggle(&p,2,60)==1);
 assert(etch_seq_hold_span(&p,16,0,60)==3);
 etch_seq_layers(&p,16,1,&layer);assert(layer.hold_pitches==1&&layer.trigger_count==0);
 assert(etch_seq_put(&p,1,60,127,1));etch_seq_layers(&p,16,1,&layer);
 assert(layer.hold_pitches==1&&layer.trigger_count==1&&layer.overlap_pitches==1);
 assert(etch_seq_hold_toggle(&p,2,60)==2&&etch_seq_hold_span(&p,16,0,60)==2);
 assert(etch_seq_hold_toggle(&p,2,60)==1&&etch_seq_hold_toggle(&p,3,60)==1);
 assert(etch_seq_hold_span(&p,16,0,60)==4);
 /* Atomic chord BAKE preserves previous note and respects eight-note limit. */
 struct etch_harmony_settings harm={0,0,2,2};
 assert(etch_seq_put(&p,4,55,100,1));assert(etch_seq_bake_chord(&p,4,60,&harm,110,2)==4);
 assert(p.steps[4].count==5&&p.steps[4].notes[0].note==55);
 for(uint8_t i=0;i<3;i++)assert(etch_seq_put(&p,4,(uint8_t)(84+i),100,1));
 struct etch_seq_step before=p.steps[4];
 assert(!etch_seq_bake_chord(&p,4,72,&harm,127,1));
 assert(!memcmp(&before,&p.steps[4],sizeof(before)));
 /* Only three additional fit because one chord already added four, old one remains. */
 puts("Native HOLD/TRIGGER: wrap-around, same-pitch overlap, independent toggle, bounded layer counts PASS");
}
