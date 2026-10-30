/* SPDX-License-Identifier: GPL-3.0-only. Eight-voice audio-rate WAVE AMP integration. */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "audio_core.h"
static struct etch_native_audio audio;
static int16_t a[128],baseline[128];
static struct etch_native_controls controls;
static uint64_t render_case(uint8_t mode,uint16_t depth,uint8_t poly){
 etch_native_reset(&audio);
 memset(&controls,0,sizeof controls);
 controls.morph=13500u;controls.depth=4096u;controls.gain=30000u;
 controls.wave_amp_mode=mode;controls.wave_amp_depth=depth;
 controls.sustain_level=30000u;
 controls.seq_gate=75u;controls.seq_length=16u;controls.tempo=120u;
 controls.filter_type=1u;controls.cutoff=15000u;
 controls.mono_mode=poly?0u:1u;
 uint32_t keys=poly?0x07ffffffu:1u;
 uint64_t energy=0u;
 for(unsigned block=0;block<16;block++){
  etch_native_render(&audio,&controls,keys,a,128);
  if(block==15u && !mode)memcpy(baseline,a,sizeof a);
  if(block>=4u)for(unsigned i=0;i<128;i++){
   int32_t s=a[i];if(s<0)s=-s;assert(s<=32767u);energy+=(uint32_t)s;
  }
 }
 return energy;
}
int main(void){
 uint64_t dry=render_case(0,0,0);assert(dry>5000u);
 int16_t old[128];memcpy(old,baseline,sizeof old);
 assert(render_case(0,32768,0)==dry);
 assert(!memcmp(old,baseline,sizeof old));
 assert(render_case(1,0,0)==dry);
 uint64_t ea=render_case(1,32768,0);
 uint64_t eb=render_case(2,32768,0);
 uint64_t em=render_case(3,32768,0);
 assert(ea>1000u&&eb>1000u&&em>1000u);
 assert(ea!=dry && eb!=dry && em!=dry);
 assert(ea!=eb || eb!=em);
 uint64_t poly_dry=render_case(0,0,1);
 uint64_t poly_amp=render_case(3,32768,1);
 assert(poly_dry>10000u && poly_amp>10000u && poly_dry!=poly_amp);
 printf("PASS V1.84 native PCM: off=%llu A=%llu B=%llu MORPH=%llu poly_off=%llu poly_on=%llu\n",
 (unsigned long long)dry,(unsigned long long)ea,(unsigned long long)eb,(unsigned long long)em,
 (unsigned long long)poly_dry,(unsigned long long)poly_amp);
 return 0;
}
