/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
static struct etch_native_audio audio;
static struct etch_native_controls ctl;
static int16_t out[128];
static uint8_t phrase_diff(uint16_t a,uint16_t b,uint8_t base,const struct etch_harmony_settings *h,uint8_t oct){
 uint8_t n=0;
 for(uint8_t i=0;i<16;i++)n+=(etch_riff_pick(a,i,base,h,oct)!=etch_riff_pick(b,i,base,h,oct));
 return n;
}
int main(void){
 const struct etch_harmony_settings h={0,0,0,0};
 for(uint16_t seed=0;seed<1000;seed+=41){
  uint16_t next=etch_riff_generate_distinct(seed,60,&h,1);
  assert(next!=seed);
  assert(phrase_diff(seed,next,60,&h,1)>=4u);
  assert(etch_riff_generate_distinct(seed,60,&h,1)==next);
 }
 memset(&audio,0,sizeof(audio));memset(&ctl,0,sizeof(ctl));
 etch_native_reset(&audio);
 ctl.gain=32767;ctl.tempo=120;ctl.arp_rate=0;ctl.arp_mode=7;
 ctl.arp_octaves=1;ctl.riff_density=50;ctl.harmony=h;
 ctl.riff_seed=0xabcdu;
 etch_native_render(&audio,&ctl,1u<<7,out,128);
 assert(audio.perf_last_arp==1u);
 assert(audio.perf_index==1u);
 assert(audio.random_seed==ctl.riff_seed);
 /* A riff change must restart before the old >1000-sample clock expires. */
 const uint16_t next=etch_riff_generate_distinct((uint16_t)ctl.riff_seed,60,&h,1);
 ctl.riff_seed=next;
 etch_native_render(&audio,&ctl,1u<<7,out,128);
 assert(audio.perf_index==1u);
 assert(audio.random_seed==ctl.riff_seed);
 /* No new note or riff on the following render -- page switches have no
  * DSP-side meaning, so the selected seed/phrase must remain unchanged. */
 etch_native_render(&audio,&ctl,1u<<7,out,128);
 assert(audio.perf_index==1u && audio.random_seed==ctl.riff_seed);
 /* Switching off and on preserves seed; changes are not stored in an
  * ephemeral screen-only variable. */
 etch_native_render(&audio,&ctl,0u,out,128);
 etch_native_render(&audio,&ctl,1u<<7,out,128);
 assert(ctl.riff_seed==next && audio.perf_last_arp==1u);
 puts("PASS V1.36 K4 distinct riff, immediate live reset and stability");
 return 0;
}
