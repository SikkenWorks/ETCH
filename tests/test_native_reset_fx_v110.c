/* V1.10 cold/warm audio startup: .noinit FX lines and EQ must be initialized. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/audio_core.h"
static struct etch_native_audio voice;
int main(void){
 etch_native_reset(&voice);
 for(int k=0;k<3;k++)assert(voice.fx.eq_gain_smooth[k]==16384);
 assert(voice.fx.comp_gain==32767&&voice.fx.comp_target==32767);
 assert(voice.poly_div_q10==4096);
 assert(voice.fx.delay_pos==0&&voice.fx.chorus_pos==0);
 struct etch_fx_settings dry;etch_fx_defaults(&dry);
 for(int k=0;k<10000;k++){
  int16_t in=(int16_t)(((k*1103)%57300)-28650),l,r;
  etch_fx_process_stereo_ex(&voice.fx,in,0,0,0,&dry,&l,&r);
  assert(l==in&&r==in);
 }
 struct etch_fx_settings wet=dry;wet.flanger_mix=26000;
 for(int k=0;k<4000;k++){
  int16_t l,r;
  etch_fx_process_stereo_ex(&voice.fx,(int16_t)(k%100*125),0,0,0,&wet,&l,&r);
 }
 etch_native_reset(&voice);
 assert(voice.fx.flanger_mix_smooth==0);
 for(int k=0;k<3;k++)assert(voice.fx.eq_gain_smooth[k]==16384);
 assert(voice.fx.comp_gain==32767&&voice.fx.comp_target==32767);
 assert(voice.fx.delay_pos==0&&voice.fx.chorus_pos==0);
 puts("PASS V1.10 FX startup/warm-reset neutral state and dry sample exactness");
}
