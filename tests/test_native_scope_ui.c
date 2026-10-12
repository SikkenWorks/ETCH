/* ETCH V0.71: real-time UI trigger and effect parameter invalidation. */
#include "../firmware/fm1-native/native_ui_wave.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void){
 assert(etch_ui_live_frame_interval(8,0,0)==50u);
 assert(etch_ui_live_frame_interval(8,1,0)==80u);
 assert(etch_ui_live_frame_interval(8,3,0)==120u);
 assert(etch_ui_live_frame_interval(4,0,0)==50u);
 assert(etch_ui_live_frame_interval(8,0,1)==750u);
 int16_t samples[512];for(unsigned i=0;i<512;i++)samples[i]=(int16_t)(((int)i%64-32)*350);
 uint16_t start=etch_ui_scope_trigger(samples,512,384);
 assert(start>=1 && start<=128 && samples[start-1]<0 && samples[start]>=0);
 uint16_t zeros[512]={0};assert(etch_ui_scope_trigger(zeros,512,384)==0);
 uint16_t v[]={0,16384,12345,25000,1234,32000,3300,16000};
 uint32_t a=etch_ui_values_fingerprint(v,8);
 for(unsigned i=0;i<8;i++){unsigned old=v[i];v[i]++;assert(etch_ui_values_fingerprint(v,8)!=a);v[i]=(uint16_t)old;}
 assert(etch_ui_wave_display_gain(zeros,128)==256);

 /* A quieter MASTER must render a genuinely smaller trace. Gain must not
  * normalize away a halved output level on an identical source signal. */
 uint16_t z=768u;
 for(int k=0;k<18;k++)z=etch_ui_scope_master_zoom(12000u,32767u,z);
 uint16_t zh=768u;
 for(int k=0;k<18;k++)zh=etch_ui_scope_master_zoom(6000u,16384u,zh);
 assert(zh==z || (int)zh-(int)z<=2);
 assert((6000u*(uint32_t)zh)<(12000u*(uint32_t)z));
 /* ARP rests do not blow the zoom up to max or collapse it to silence. */
 assert(etch_ui_scope_master_zoom(0u,32767u,z)==z);
 assert(etch_ui_scope_master_zoom(2u,32767u,z)==z);
 assert(etch_ui_scope_master_zoom(0u,0u,z)==z);
 uint16_t vstrong=etch_ui_scope_master_zoom(3000u,32767u,768u);
 assert(vstrong>768u); /* quieter tone gets visibly bigger than V1.08 */
 puts("PASS live scope zero crossing and 8 effect parameter change invalidations");
}
