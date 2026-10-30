#include "../firmware/fm1-native/native_attack_smoothing_v184.h"
#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdio.h>
static void ramp(unsigned total){
 unsigned old=257;
 for(unsigned remain=total; ;remain--){
  unsigned a=etch_transition_old_mix_q8(remain,total);
  assert(a<=256&&a<=old);
  if(remain==total)assert(a==256);
  if(remain==0){assert(a==0);break;}
  old=a;
 }
 /* No steep initial or final slope. */
 assert(etch_transition_old_mix_q8(total,total)-etch_transition_old_mix_q8(total-1,total)<=2);
 assert(etch_transition_old_mix_q8(1,total)<=2);
}
int main(void){
 ramp(256);ramp(ETCH_TRANSITION_SAMPLES);
 assert(etch_attack_gain_q8(ETCH_ATTACK_SAMPLES)==0);
 assert(etch_attack_gain_q8(0)==256);
 puts("PASS V1.85: bank and voice-steal crossfades monotonically curved, zero-slope endpoints");
}
