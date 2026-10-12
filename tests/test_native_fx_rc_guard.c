/* V0.56 F03: 44.1 kHz native FX and high-drive release guard test.
 * An offline host clock cannot certify FM1 CPU or DAC; those stay PARTIAL.
 */
#include "../firmware/fm1-native/native_fx.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
static struct etch_native_fx fx;
static uint32_t mag(int16_t x){return x<0?(uint32_t)(-(int32_t)x):(uint32_t)x;}
int main(void){
 const uint32_t samples=44100*5;
 uint64_t total=0;uint32_t peak=0,clipped=0,diverged=0;
 for(unsigned route=0;route<2;route++)for(unsigned level=0;level<3;level++){
  etch_fx_reset(&fx);struct etch_fx_settings cfg;etch_fx_defaults(&cfg);
  cfg.routing=(uint8_t)route;cfg.chorus_rate=32767;cfg.chorus_depth=32767;
  cfg.delay_feedback=32767;cfg.reverb_decay=32767;cfg.reverb_size=32767;
  uint16_t wet=level==0?0:level==1?12000:32767;
  for(uint32_t i=0;i<samples;i++){
   int16_t in=(int16_t)((i%97)<48?30000:-30000);
   int16_t l=0,r=0;
   etch_fx_process_stereo_ex(&fx,in,wet,wet,wet,&cfg,&l,&r);
   if(!wet)assert(l==in && r==in);
   if(wet){assert(mag(l)<=30192 && mag(r)<=30192);if(l!=r)diverged++;}
   if(mag(l)>=32767 || mag(r)>=32767)clipped++;
   if(mag(l)>peak)peak=mag(l);
   if(mag(r)>peak)peak=mag(r);
   total+=(uint64_t)mag(l)+mag(r);
  }
 }
 assert(peak<=30192 && clipped==0 && diverged>0 && total>0);
 printf("PASS F03 native FX 5s x 6 routes/wet levels: peak=%u/32768, fullscale_frames=%u, stereo_diff=%u\n",peak,clipped,diverged);
}
