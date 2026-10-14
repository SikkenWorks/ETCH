/* SPDX-License-Identifier: GPL-3.0-only
 * V1.35: preset-bound EVOLUTION/ORGANIC values are bypassed in polyphony.
 * The same settings remain audible in mono, without mutating the preset.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_tables.h"
static struct etch_native_audio reference_audio,settings_audio;
static struct etch_native_controls reference_ctl,settings_ctl;
static struct etch_native_bank bank;
static int16_t ref_pcm[256],setting_pcm[256];
static void defaults(uint8_t mono){
 memset(&reference_ctl,0,sizeof reference_ctl);
 etch_fx_defaults(&reference_ctl.fx_settings);
 reference_ctl.bank=&bank;reference_ctl.mono_mode=mono;
 reference_ctl.env_enabled=1;reference_ctl.attack_step=0x7fff0000u;
 reference_ctl.decay_step=0x7fff0000u;reference_ctl.sustain_level=26000u;
 reference_ctl.release_step=0x10000000u;reference_ctl.gain=24576u;
 reference_ctl.cutoff=8192u;reference_ctl.tempo=120u;reference_ctl.morph=14000u;
 settings_ctl=reference_ctl;
 settings_ctl.evolution=(struct etch_evolution_settings){100u,96u,0u,0u};
 settings_ctl.organic=(struct etch_organic_settings){100u,100u,100u,100u};
 etch_native_reset(&reference_audio);etch_native_reset(&settings_audio);
}
int main(void){
 for(int w=0;w<3;w++)for(int m=0;m<6;m++)for(int k=0;k<512;k++)bank.table[w][m][k]=etch_tables[w][m][k];
 defaults(0);
 for(int note=0;note<8;note++){
  etch_native_midi(&reference_audio,0x90,(uint8_t)(48+note*3),112);
  etch_native_midi(&settings_audio,0x90,(uint8_t)(48+note*3),112);
 }
 uint64_t energy=0;
 for(int n=0;n<25;n++){
  etch_native_render_stereo(&reference_audio,&reference_ctl,0,ref_pcm,128);
  etch_native_render_stereo(&settings_audio,&settings_ctl,0,setting_pcm,128);
  assert(memcmp(ref_pcm,setting_pcm,sizeof ref_pcm)==0);
  for(int i=0;i<256;i++)energy+=ref_pcm[i]<0?-(int32_t)ref_pcm[i]:ref_pcm[i];
 }
 assert(energy>0);
 assert(settings_ctl.evolution.depth==100u && settings_ctl.organic.drift==100u);
 defaults(1);
 etch_native_midi(&reference_audio,0x90,52u,127u);
 etch_native_midi(&settings_audio,0x90,52u,127u);
 uint32_t difference=0;
 for(int n=0;n<45;n++){
  etch_native_render_stereo(&reference_audio,&reference_ctl,0,ref_pcm,128);
  etch_native_render_stereo(&settings_audio,&settings_ctl,0,setting_pcm,128);
  for(int i=0;i<256;i++)difference+=(uint32_t)(ref_pcm[i]!=setting_pcm[i]);
 }
 assert(difference>100);
 puts("PASS V1.35: POLY identical at 8 voices with EVOLUTION/ORGANIC presets; MONO restores both");
 return 0;
}
