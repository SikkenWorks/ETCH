/* SPDX-License-Identifier: GPL-3.0-only
 * V1.17 real host PCM confirms A and B pre-morph FX are active and
 * Evolution knobs drive audible DSP. No simulated FM1 panel involved.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_live_controls_v117.h"
static struct etch_native_audio synth;
static struct etch_native_controls c;
static uint64_t signature(uint16_t morph,uint8_t wave,uint8_t effect,uint8_t evolve){
 etch_native_reset(&synth);memset(&c,0,sizeof(c));
 etch_fx_defaults(&c.fx_settings);
 c.gain=32767;c.tempo=120;c.morph=morph;c.sustain_level=32767;
 c.env_enabled=1;c.attack_step=0x7fff0000u;c.decay_step=0x7fff0000u;c.release_step=0x00400000u;
 if(effect)etch_v117_pre_turn(&c.fx_settings,wave,0,3,60,0); /* FOLD, wet by default */
 if(evolve){struct etch_evolution_settings e={0,30,0,2};etch_v117_evolution_turn(&e,16,12,1,0,0);c.evolution=e;}
 uint64_t hash=1469598103934665603ull;
 for(unsigned b=0;b<80;b++){
  int16_t pcm[128];etch_native_render(&synth,&c,1u,pcm,128);
  for(unsigned i=0;i<128;i++)hash=(hash^(uint16_t)pcm[i])*1099511628211ull;
 }
 return hash;
}
int main(void){
 uint64_t a_dry=signature(0,0,0,0), a_fx=signature(0,0,1,0),b_fx=signature(0,1,1,0);
 uint64_t b_dry=signature(32767,1,0,0), b_wet=signature(32767,1,1,0);
 uint64_t evo=signature(0,0,0,1);
 printf("A dry=%llx A FX=%llx A/B wrong=%llx B dry=%llx B FX=%llx EVO=%llx\n",
  (unsigned long long)a_dry,(unsigned long long)a_fx,(unsigned long long)b_fx,
  (unsigned long long)b_dry,(unsigned long long)b_wet,(unsigned long long)evo);
 assert(a_dry!=a_fx);
 assert(a_dry==b_fx);
 assert(b_dry!=b_wet);
 assert(a_dry!=evo);
 puts("PASS V1.17 realtime A/B pre-FX audio paths, morph source isolation, Evolution audible render");
}
