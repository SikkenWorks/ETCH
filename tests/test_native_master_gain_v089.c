/* Regression: changing MASTER during a held note must not step the PCM amplitude. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
static struct etch_native_audio synth;
static struct etch_native_controls controls;
static int16_t pcm[256];
int main(void){
 etch_native_reset(&synth);
 assert(synth.master_gain_q16==0);
 controls.gain=0;
 controls.env_enabled=0;
 etch_native_render(&synth,&controls,0,pcm,128);
 assert(synth.master_gain_q16==0);
 controls.gain=32767;
 etch_native_render(&synth,&controls,0,pcm,128);
 assert(synth.master_gain_q16>0);
 assert(synth.master_gain_q16 < (32767*65536));
 int32_t a=synth.master_gain_q16;
 etch_native_render(&synth,&controls,0,pcm,128);
 assert(synth.master_gain_q16>a);
 assert(synth.master_gain_q16 <= 32767*65536);
 controls.gain=0;
 int32_t old=synth.master_gain_q16;
 etch_native_render(&synth,&controls,0,pcm,128);
 assert(synth.master_gain_q16<old && synth.master_gain_q16>0);
 /* Fast arbitrary ADC changes always bound master to 0..0x7fff. */
 for(int i=0;i<500;i++){
  controls.gain=(i&1)?0:32767;
  etch_native_render(&synth,&controls,0,pcm,128);
  assert(synth.master_gain_q16>=0 && synth.master_gain_q16<=32767*65536);
 }
 puts("PASS V0.89: master gain is sample-smoothed across blocks with bounded ADC changes");
}
