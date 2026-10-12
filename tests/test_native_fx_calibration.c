/* V0.53 offline FX headroom and stereo separation sweep. This cannot replace
 * on-device CPU, DAC, USB or listening measurements. */
#include "../firmware/fm1-native/native_fx.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static struct etch_native_fx f;
static int32_t abs16(int32_t n){return n<0?-n:n;}
int main(void){
 unsigned scenarios=0;uint32_t total_divergent=0,peak=0;
 for(unsigned route=0;route<2;route++)for(unsigned wet=0;wet<3;wet++)for(unsigned feedback=0;feedback<3;feedback++){
  etch_fx_reset(&f);
  struct etch_fx_settings c;etch_fx_defaults(&c);
  c.routing=(uint8_t)route;
  c.chorus_rate=(uint16_t)(1000+10000*wet);
  c.chorus_depth=(uint16_t)(7000+9000*wet);
  c.delay_time=(uint16_t)(4000+12000*wet);
  c.delay_feedback=(uint16_t)(10000+10000*feedback);
  c.reverb_size=(uint16_t)(5000+11000*wet);
  c.reverb_decay=(uint16_t)(12000+9000*feedback);
  uint32_t divergent=0;
  for(int sample=0;sample<50000;sample++){
   int32_t phase=sample%128;
   int16_t v=(int16_t)(sample<25000?(phase<64?(int32_t)phase*360-11520:((127-(int32_t)phase)*360-11520)):0);
   int16_t left=0,right=0;
   etch_fx_process_stereo_ex(&f,v,32767,32767,32767,&c,&left,&right);
   uint32_t al=(uint32_t)abs16(left),ar=(uint32_t)abs16(right);
   if(al>peak)peak=al;if(ar>peak)peak=ar;
   assert(al<=32768u && ar<=32768u);
   if(left!=right)divergent++;
  }
  assert(divergent>1000u);total_divergent+=divergent;scenarios++;
 }
 printf("PASS V0.53 FX stereo/feedback sweep: %u scenarios; divergent=%u frames; peak=%u/32768 (offline, not device-calibrated)\n",scenarios,total_divergent,peak);
}
