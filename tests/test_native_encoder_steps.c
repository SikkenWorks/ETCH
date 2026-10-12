#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_encoder_steps.h"
int main(void){
 for(int n=0;n<=100;n++){
  uint16_t raw=(uint16_t)(((uint32_t)n*32767u+99u)/100u);
  assert(raw*100u/32767u==(uint32_t)n);
  if(n<100){uint16_t plus=etch_encoder_pct(raw,1);assert((uint32_t)plus*100u/32767u==(uint32_t)n+1u);}
  if(n>0){uint16_t minus=etch_encoder_pct(raw,-1);assert((uint32_t)minus*100u/32767u==(uint32_t)n-1u);}
 }
 assert(etch_encoder_pct(32767u,50)==32767u);
 assert(etch_encoder_pct(0u,-50)==0u);
 assert(etch_encoder_unit(999u,1,0,10000)==1000u);
 assert(etch_encoder_unit(400u,-1,16,8192)==399u);
 assert(etch_encoder_hz(175000u,1)==176000u);
 assert(etch_encoder_hz(175000u,-1)==174000u);
 /* Every cutoff control surface uses exactly 20 displayed units per detent.
  * Existing stored values are not rounded, and end stops are saturated. */
 assert(etch_encoder_cutoff(1234,1)==1254);
 assert(etch_encoder_cutoff(1234,-1)==1214);
 assert(etch_encoder_cutoff(1234,5)==1334);
 assert(etch_encoder_cutoff(1234,-5)==1134);
 assert(etch_encoder_cutoff(16,-1)==16);
 assert(etch_encoder_cutoff(19,-1)==16);
 assert(etch_encoder_cutoff(8185,1)==8192);
 assert(etch_encoder_cutoff(8192,1)==8192);
 assert(etch_encoder_cutoff(8192,-1)==8172);
 assert(etch_encoder_cutoff(7000,2147483647)==8192);
 assert(etch_encoder_cutoff(7000,-2147483647-1)==16);
 for(uint8_t i=0;i<6u;i++){
  assert(etch_encoder_lock_value(500,i,1)==(i==2u?520u:501u));
  assert(etch_encoder_lock_value(500,i,-1)==(i==2u?480u:499u));
 }
 assert(etch_encoder_lock_value(0,2,-1)==0);
 assert(etch_encoder_lock_value(32765,2,1)==32767);
 assert(etch_encoder_lock_value(32760,1,1)==32761);
 /* Library K3/K4 must have no action; K1/K2 remain 1-step controls. */
 struct etch_library_encoder_state st;
 st=etch_encoder_library_controls(16,1,0,0,100,-100);
 assert(st.slot==16 && st.target==1);
 st=etch_encoder_library_controls(16,1,1,0,0,0);
 assert(st.slot==17 && st.target==1);
 st=etch_encoder_library_controls(16,1,-1,1,0,0);
 assert(st.slot==15 && st.target==2);
 st=etch_encoder_library_controls(16,1,0,-1,-8,8);
 assert(st.slot==16 && st.target==0);
 st=etch_encoder_library_controls(63,2,20,20,0,0);
 assert(st.slot==63 && st.target==2);
 st=etch_encoder_library_controls(0,0,-20,-20,0,0);
 assert(st.slot==0 && st.target==0);
 puts("Encoder: cutoff +20; other controls +1 per detent, library K3/K4 disabled");
}
