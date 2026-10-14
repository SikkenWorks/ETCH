/* ETCH V0.70: realistic note pressure and the deadline-triggered voice budget.
 * This runs on the host; physical DMA load must still be measured on the FM1.
 */
#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct etch_native_audio synth;
static int16_t output[ETCH_NATIVE_FRAMES*2];
int main(void){
 etch_native_reset(&synth);
 assert(synth.voice_limit==8u);
 struct etch_native_controls c={0};c.gain=16384;c.morph=8192;
 c.attack_step=0x40000000u;c.decay_step=0x40000000u;c.release_step=0x10000000u;
 c.sustain_level=26000;c.env_enabled=1;c.cutoff=6000;c.filter_type=1;
 for(unsigned round=0;round<15;round++){
  /* All 27 keyboard bits pressed within one ISR block, also held across it. */
  uint32_t mask=(round&1u)?0x07ffffffu:0x04ffffffu;
  etch_native_render_stereo(&synth,&c,mask,output,ETCH_NATIVE_FRAMES);
  unsigned active=0;for(unsigned k=0;k<8;k++)active+=synth.voice[k].active?1:0;
  assert(active<=synth.voice_limit);
 }
 assert(etch_voice_budget_reduce(synth.voice_limit)==6u);
 synth.voice_limit=etch_voice_budget_reduce(synth.voice_limit);
 etch_native_render_stereo(&synth,&c,0x07ffffffu,output,ETCH_NATIVE_FRAMES);
 for(unsigned k=6;k<8;k++)assert(!synth.voice[k].active);
 synth.voice_limit=etch_voice_budget_reduce(synth.voice_limit);
 etch_native_render_stereo(&synth,&c,0x07ffffffu,output,ETCH_NATIVE_FRAMES);
 for(unsigned k=4;k<8;k++)assert(!synth.voice[k].active);
 synth.voice_limit=etch_voice_budget_reduce(synth.voice_limit);
 assert(synth.voice_limit==3u);
 etch_native_render_stereo(&synth,&c,0x07ffffffu,output,ETCH_NATIVE_FRAMES);
 synth.voice_limit=etch_voice_budget_reduce(synth.voice_limit);
 assert(synth.voice_limit==2u);
 etch_native_render_stereo(&synth,&c,0x07ffffffu,output,ETCH_NATIVE_FRAMES);
 for(unsigned k=2;k<8;k++)assert(!synth.voice[k].active);
 synth.voice_limit=etch_voice_budget_restore(synth.voice_limit);
 assert(synth.voice_limit==3u);
 synth.voice_limit=etch_voice_budget_restore(synth.voice_limit);
 assert(synth.voice_limit==4u);
 synth.voice_limit=etch_voice_budget_restore(synth.voice_limit);
 assert(synth.voice_limit==6u);
 synth.voice_limit=etch_voice_budget_restore(synth.voice_limit);
 assert(synth.voice_limit==8u);
 for(unsigned k=0;k<1000;k++){
  etch_native_render_stereo(&synth,&c,(k&15u)?0x07ffffffu:0u,output,ETCH_NATIVE_FRAMES);
  unsigned active=0;for(unsigned j=0;j<8;j++)active+=synth.voice[j].active?1:0;
  assert(active<=synth.voice_limit);
 }
 puts("PASS 27 simultaneous keys, adaptive 8->6->4->3->2 budget, soft tails, 8 restoration, 1000 stereo render blocks");
}
