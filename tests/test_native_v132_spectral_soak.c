/* SPDX-License-Identifier: GPL-3.0-only
 * A spectral edit may recompile waveform banks while eight MIDI voices play.
 * Assert sustained audio, bounded PCM and no permanently stale crossfade.
 * Host-only: cannot verify physical pi32v2 DMA or codec pop.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_spectral_controls_v131.h"
static struct etch_native_audio s;
static struct etch_native_controls c;
static struct etch_wave_editor editor;
static struct etch_native_bank banks[2];
static int16_t samples[ETCH_NATIVE_FRAMES*2u];
static unsigned long sounds,clipped;
static void render_n(unsigned blocks){
 for(unsigned i=0;i<blocks;i++){
  etch_native_render_stereo(&s,&c,0,samples,ETCH_NATIVE_FRAMES);
  for(unsigned j=0;j<ETCH_NATIVE_FRAMES*2u;j++){
   int32_t v=samples[j];
   assert(v>=-32768&&v<=32767);
   if(v) sounds++;
   if(v==32767||v==-32768)clipped++;
  }
 }
}
int main(void){
 etch_editor_init(&editor);
 etch_editor_compile(&editor,&banks[0]);
 etch_native_reset(&s);
 memset(&c,0,sizeof(c));etch_fx_defaults(&c.fx_settings);
 c.bank=&banks[0];c.gain=24000;c.tempo=120;c.morph=16000;
 c.env_enabled=1;c.attack_step=0x7fff0000u;c.decay_step=0x7fff0000u;
 c.sustain_level=27000;c.release_step=0x01000000u;
 c.living.depth=95;c.living.rate=95;c.living.color=10;c.living.vary=80;
 c.organic.drift=40;c.organic.rate=70;c.organic.variance=30;
 for(unsigned i=0;i<8;i++)etch_native_midi(&s,0x90,(uint8_t)(45+i*4),110);
 render_n(20);
 editor.wave=0;
 uint8_t gain=50,tilt=50;
 etch_spectral_controls_turn(&editor,4,&gain,&tilt,15,-20);
 assert(editor.dirty);
 etch_editor_compile(&editor,&banks[1]);
 c.bank=&banks[1];
 render_n(20);
 assert(s.bank_fade==0u && s.old_bank==NULL);
 render_n(80);
 for(unsigned i=0;i<8;i++)etch_native_midi(&s,0x80,(uint8_t)(45+i*4),0);
 render_n(50);
 unsigned long before=sounds;
 for(unsigned i=0;i<8;i++)etch_native_midi(&s,0x90,(uint8_t)(48+i*4),112);
 render_n(30);
 assert(sounds>before+100u);
 assert(clipped<100u); /* no continuous full-scale clipping after spectral commit */
 puts("PASS 8-voice SPECTRAL bank publish, LIVING and ORGANIC, no stale crossfade or runaway clipping");
}
