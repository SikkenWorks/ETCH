/* ETCH V1.17: all-notes-off must end with a flat LIVE trace. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_ui_scope_policy.h"
#include "../firmware/fm1-native/native_ui_wave.h"
int main(void){
 struct etch_scope_release_state s={0};
 /* Idle, including a ringing FX buffer, never animates indefinitely. */
 assert(etch_scope_release_step(&s,0,0)==0);
 assert(etch_scope_visual_gain(2048,0,1)==0);
 assert(!etch_scope_anim_needed(&s,0,1,1));
 /* A physical key, an external MIDI key or an ARP key is one held voice. */
 assert(etch_scope_anim_needed(&s,1,1,1));
 assert(etch_scope_release_step(&s,1,1000)==256);
 assert(s.had_held_note&&!s.releasing);
 assert(etch_scope_visual_gain(1024,256,1)==1024);
 /* Release starts a short fade, not an abrupt jump. */
 assert(etch_scope_anim_needed(&s,0,1,1));
 assert(etch_scope_release_step(&s,0,1020)==256);
 assert(s.releasing);
 uint16_t previous=256;
 for(uint32_t delta=20;delta<ETCH_SCOPE_RELEASE_MS;delta+=20){
  uint16_t a=etch_scope_release_step(&s,0,1020+delta);
  assert(a<previous);
  assert(a>0);
  assert(etch_scope_visual_gain(2048,a,1)>0);
  previous=a;
 }
 /* Even a loud self-oscillating feedback tail must end at exactly center. */
 assert(etch_scope_release_step(&s,0,1280)==0);
 assert(etch_scope_visual_gain(2048,0,1)==0);
 assert(!etch_scope_anim_needed(&s,0,1,1));
 int16_t leftover[]={-32000,28000,5000,25000};
 for(int j=0;j<4;j++)assert(etch_ui_wave_display_y(leftover[j],
   etch_scope_visual_gain(2048,0,1),98,51,43,153)==98);
 /* Silent PCM also clears immediately, and this never changes audio PCM. */
 assert(etch_scope_release_step(&s,1,1450)==256);
 assert(etch_scope_visual_gain(2000,256,0)==0);
 assert(etch_scope_release_step(&s,0,1500)==256);
 assert(etch_scope_release_step(&s,1,1550)==256); /* retrigger interrupts fade */
 assert(!s.releasing);
 assert(etch_scope_release_step(&s,0,1600)==256);
 assert(etch_scope_release_step(&s,0,1860)==0);
 /* Monotonic tick wrap is handled by unsigned subtraction. */
 assert(etch_scope_release_step(&s,1,0xfffffff0u)==256);
 assert(etch_scope_release_step(&s,0,0xfffffff0u)==256);
 assert(etch_scope_release_step(&s,0,0x000000f4u)==0);
 puts("PASS V1.17 gate-aware scope fade, exact flat zero, FX tail, retrigger, rollover");
 return 0;
}
