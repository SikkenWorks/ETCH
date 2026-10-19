/* SPDX-License-Identifier: GPL-3.0-only
 * Lossless resource optimization PCM comparison. Compile once with the
 * unmodified V1.44 audio_core.c and again with the optimized audio_core.c.
 * All printed hashes must match exactly, including enabled Wave-FX and
 * three concurrently sounding multitimbral parts.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_tables.h"

static struct etch_native_audio audio;
static struct etch_native_controls controls;
static struct etch_key_groups groups;
static struct etch_native_patch patches[ETCH_PATCH_SLOTS];
static struct etch_fx_settings group_fx[ETCH_PATCH_SLOTS];
static struct etch_preset_dsp group_dsp[ETCH_PATCH_SLOTS];
static int16_t pcm[ETCH_NATIVE_FRAMES*2u];

const struct etch_fx_settings *etch_group_fx_for_preset(uint8_t p){return p<ETCH_PATCH_SLOTS?&group_fx[p]:0;}
uint8_t etch_group_velocity_for_preset(uint8_t p){return p==4u?1u:p==9u?2u:0u;}
const struct etch_preset_dsp *etch_group_dsp_for_preset(uint8_t p){return p<ETCH_PATCH_SLOTS?&group_dsp[p]:0;}
static void setup(uint8_t with_groups,uint8_t with_wave_fx,uint8_t with_master_fx){
 memset(&controls,0,sizeof controls);
 memset(patches,0,sizeof patches);
 memset(group_dsp,0,sizeof group_dsp);
 etch_native_reset(&audio);
 etch_fx_defaults(&controls.fx_settings);
 controls.gain=24576u;
 controls.env_enabled=1u;controls.attack_step=0x7fff0000u;
 controls.decay_step=0x7fff0000u;controls.release_step=0x00800000u;
 controls.sustain_level=21000u;controls.morph=14500u;
 controls.depth=13000u;controls.filter_type=1u;
 controls.cutoff=5000u;controls.resonance=8000u;
 controls.mod_ratio=2u;controls.mod_target=1u;
 controls.tempo=120;
 if(with_master_fx){controls.chorus=12000u;controls.delay=9000u;controls.reverb=13000u;}
 if(with_wave_fx){
  etch_pre_set_type(&controls.fx_settings,0u,0u,1u);
  etch_pre_set_depth(&controls.fx_settings,0u,0u,10500u);
  etch_pre_set_mix(&controls.fx_settings,0u,0u,18000u);
  etch_pre_set_type(&controls.fx_settings,1u,1u,5u);
  etch_pre_set_depth(&controls.fx_settings,1u,1u,22000u);
  etch_pre_set_mix(&controls.fx_settings,1u,1u,13000u);
 }
 etch_key_groups_init(&groups);
 for(unsigned p=0;p<ETCH_PATCH_SLOTS;p++){
  etch_fx_defaults(&group_fx[p]);
 }
 if(with_groups){
  assert(etch_key_group_set(&groups,0u,4u,36u,49u,1u));
  assert(etch_key_group_set(&groups,1u,9u,60u,73u,1u));
  for(unsigned p=4u;p<=9u;p+=5u){
   patches[p].valid=1u;
   patches[p].cutoff=(uint16_t)(p==4u?3200u:7500u);
   patches[p].resonance=(uint16_t)(p==4u?12000u:25000u);
   patches[p].filter_type=p==4u?1u:3u;
   patches[p].morph=p==4u?31000u:1000u;
   patches[p].depth=p==4u?11000u:21000u;
   patches[p].sustain=25500u;
   patches[p].attack_ms=5u;patches[p].decay_ms=200u;patches[p].release_ms=350u;
   patches[p].mod_ratio=p==4u?2u:5u;
   patches[p].drive=p==4u?6000u:18000u;
  }
  if(with_wave_fx){
   etch_pre_set_type(&group_fx[4u],0u,0u,3u);
   etch_pre_set_mix(&group_fx[4u],0u,0u,12000u);
   etch_pre_set_type(&group_fx[9u],1u,0u,4u);
   etch_pre_set_mix(&group_fx[9u],1u,0u,11000u);
  }
  /* Distinct wave banks, no extra allocations in the audio callback. */
  for(unsigned g=0;g<3;g++)for(unsigned w=0;w<3;w++)for(unsigned l=0;l<6;l++)for(unsigned i=0;i<512;i++)
   native_banks[g].table[w][l][i]=etch_tables[(w+g)%3u][l][i];
  controls.bank=&native_banks[0];
  etch_native_groups_patch_bank(patches);
  etch_native_groups_configure(&groups);
 }else{
  etch_native_groups_patch_bank(0);
  etch_native_groups_configure(0);
 }
}
int main(void){
 for(unsigned scenario=0;scenario<4u;scenario++){
  setup((uint8_t)(scenario>=2u),(uint8_t)(scenario&1u),(uint8_t)(scenario==1u||scenario==3u));
  const uint8_t notes[]={38,43,46,62,66,71,83,88};
  for(unsigned i=0;i<8u;i++)etch_native_midi(&audio,(uint8_t)(0x90u+i%3u),notes[i],(uint8_t)(88u+i*4u));
  uint32_t checksum=2166136261u;
  for(unsigned block=0;block<120u;block++){
   if(block==80u)etch_native_midi(&audio,0x80u,38u,0u);
   if(block==90u)controls.cutoff=6500u;
   etch_native_render_stereo(&audio,&controls,0u,pcm,ETCH_NATIVE_FRAMES);
   for(unsigned j=0;j<ETCH_NATIVE_FRAMES*2u;j++){
    checksum^=(uint16_t)pcm[j];checksum*=16777619u;
   }
  }
  static const uint32_t v144_reference[]={0x3d41dc8fu,0x774f17ebu,0x22ccfae5u,0x977419b8u};
  assert(checksum==v144_reference[scenario] && "V1.44 PCM must be bit-exact");
  printf("PASS scenario=%u checksum=%08x (V1.44 bit-exact)\n",scenario,checksum);
 }
 return 0;
}
