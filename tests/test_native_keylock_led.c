#include "../firmware/fm1-native/native_led_policy.h"
#include <assert.h>
#include <stdio.h>
static int count_bits(uint32_t v){int count=0;while(v){count+=(int)(v&1u);v>>=1;}return count;}
int main(void){
 struct etch_harmony_settings h={0,0,2,0}; /* C major, LOCK */
 /* Panel starts at MIDI F (53). F# snaps DOWN to F in C major. */
 assert(etch_quantize_note(54,&h)==53);
 assert(etch_led_keylock_targets(1u<<1,0,&h)==(1u<<0));
 /* The UI ORs the physical key with snapped target. */
 assert(((1u<<1)|etch_led_keylock_targets(1u<<1,0,&h))==3u);
 /* Key-up removes the target. */
 assert(etch_led_keylock_targets(0u,0,&h)==0u);
 /* D major: low F snaps to E, which is outside the 27-key panel.
  * Do not fake an E somewhere else on the keyboard. */
 h.root=2;
 assert(etch_quantize_note(53,&h)==52);
 assert(etch_led_keylock_targets(1u<<0,0,&h)==0u);
 h.root=0;
 /* F and F# both sound F: releasing either one keeps the shared F
  * indicator on while the other finger is held. */
 uint32_t a=etch_led_keylock_targets((1u<<0)|(1u<<1),0,&h);
 uint32_t b=etch_led_keylock_targets(1u<<1,0,&h);
 uint32_t c=etch_led_keylock_targets(1u<<0,0,&h);
 assert(a==1u && b==1u && c==1u);
 /* Guide or Off MUST NOT invent an audible target. */
 h.keylock=0;assert(etch_led_keylock_targets(1u<<1,0,&h)==0);
 h.keylock=1;assert(etch_led_keylock_targets(1u<<1,0,&h)==0);
 /* Match quantization for every scale, root, keyboard key, octave. */
 h.keylock=2;
 for(int oct=-4;oct<=3;oct++)for(int root=0;root<12;root++)for(int mode=0;mode<9;mode++){
  h.root=(uint8_t)root;h.mode=(uint8_t)mode;
  for(unsigned k=0;k<27;k++){
   int base=53+12*oct;int pitch=base+(int)k;
   if(pitch<0)pitch=0;if(pitch>127)pitch=127;
   int tuned=etch_quantize_note((uint8_t)pitch,&h);
   int idx=tuned-base;
   uint32_t expected=(idx>=0&&idx<27)?(1u<<(unsigned)idx):0;
   uint32_t actual=etch_led_keylock_targets(1u<<k,(int8_t)oct,&h);
   assert(actual==expected);
   assert(count_bits(actual)<=1);
  }
 }
 /* Edge: off-panel target never appears at an unrelated octave. */
 h.root=0;h.mode=0;
 uint32_t all=etch_led_keylock_targets(0x07ffffffu,3,&h);
 assert(!(all&~0x07ffffffu));
 /* GUIDE mode illuminates every scale pitch across all 27 real keys. */
 h.keylock=1;
 for(int oct=-4;oct<=3;oct++)for(int root=0;root<12;root++)for(int mode=0;mode<9;mode++){
  h.root=(uint8_t)root;h.mode=(uint8_t)mode;h.chord=0;
  struct etch_harmony_guide scale;assert(!etch_harmony_guide_build(&h,60,&scale));
  uint32_t expected=0;
  for(unsigned k=0;k<27;k++){
   int note=53+12*oct+(int)k;
   if(note>=0&&note<=127&&(scale.scale_mask&(1u<<(note%12))))
    expected|=1u<<k;
  }
  uint32_t actual=etch_led_scale_guide_targets((int8_t)oct,&h);
  assert(actual==expected);
  assert(!(actual&~0x07ffffffu));
  assert(count_bits(actual)>=14&&count_bits(actual)<=17);
 }
 /* Every chosen C major pitch lights across all 27 keys, not just C's.
  * Panel MIDI 53 starts on F3. */
 h.root=0;h.mode=0;h.chord=0;
 uint32_t guide=etch_led_scale_guide_targets(0,&h);
 assert((guide&(1u<<0))!=0); /* F */
 assert((guide&(1u<<1))==0); /* F# excluded */
 assert((guide&(1u<<7))!=0); /* C */
 assert((guide&(1u<<19))!=0);/* C in next octave */
 h.keylock=0;assert(!etch_led_scale_guide_targets(0,&h));
 h.keylock=2;assert(!etch_led_scale_guide_targets(0,&h));
 assert(!etch_led_scale_guide_targets(0,0));
 h.keylock=1;h.mode=9;assert(!etch_led_scale_guide_targets(0,&h));
 puts("PASS KEYLOCK LED + GUIDE lights all scale pitches across 27 physical keys, all roots/scales/octaves, OFF and LOCK unaffected");
 return 0;
}
