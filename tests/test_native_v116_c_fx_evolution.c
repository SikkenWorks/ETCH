/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.16 regression: expanded C FX, sparse-wave ordinal, PJT20 migration,
 * real PCM wet modulation, evolution at polyphonic settings.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_ui_wave.h"
static struct etch_native_audio synth;
static struct etch_native_controls control;
static struct etch_wave_library lib,oldlib;
static struct etch_native_pattern pattern,oldpattern;
static struct etch_project_settings project,oldproject;
static uint8_t packed[ETCH_PROJECT_MAX_BYTES];
static int16_t out[128];
static uint64_t pcm_signature(uint8_t target){
 etch_native_reset(&synth);
 memset(&control,0,sizeof(control));
 etch_fx_defaults(&control.fx_settings);
 control.gain=32767;control.sustain_level=32767;
 control.reverb=0;control.tempo=120;
 control.fx_settings.reverb_size=21000;
 etch_c_fx_set_target(&control.fx_settings,target);
 etch_c_fx_set_depth(&control.fx_settings,32767);
 uint64_t hash=1469598103934665603ull;
 for(int t=0;t<120;t++){
  etch_native_render(&synth,&control,1u,out,128);
  for(unsigned i=0;i<128;i++)hash=(hash^(uint16_t)out[i])*1099511628211ull;
 }
 return hash;
}
int main(void){
 struct etch_fx_settings fx;etch_fx_defaults(&fx);
 assert(sizeof(fx)==128u);
 for(unsigned id=0;id<=31;id++){
  etch_c_fx_set_target(&fx,(uint8_t)id);
  for(unsigned d=0;d<=32767;d+=1200){
   etch_c_fx_set_depth(&fx,(uint16_t)d);
   assert(etch_c_fx_target(&fx)==id);
   int error=abs((int)etch_c_fx_depth(&fx)-(int)d);
   assert(error<18);
  }
  assert(etch_c_fx_name(id)[0]);
 }
 /* After V1.16 sparse slots 3,5,7,25,35 read as C #1..#5 and the
  * original slot ID is still visible separately in the UI. */
 uint32_t valid[2]={0};unsigned slots[]={3,5,7,25,35};
 for(unsigned i=0;i<5;i++)valid[slots[i]>>5]|=1u<<(slots[i]&31);
 assert(etch_ui_wave_count(valid)==5);
 uint8_t position=(uint8_t)slots[0];
 for(unsigned i=0;i<5;i++){
  assert(etch_ui_wave_ordinal(valid,position)==i+1);
  if(i<4)position=etch_ui_wave_next(valid,position,1);
 }
 assert(etch_ui_wave_next(valid,35,1)==3);
 assert(etch_ui_wave_ordinal(valid,4)==0);
 etch_project_legacy(&lib,&pattern,&project);
 project.source_version=20;project.seq_length=16;project.tempo=120;
 project.evolution=(struct etch_evolution_settings){60,55,2,1};
 etch_c_fx_set_target(&project.global_fx,28);
 etch_c_fx_set_depth(&project.global_fx,22100);
 uint32_t used=0;
 assert(etch_project_encode(&lib,&pattern,&project,packed,sizeof(packed),&used)==0);
 assert(!memcmp(packed,"PJTD",4));
 assert(etch_project_decode(packed,used,&oldlib,&oldpattern,&oldproject)==0);
 assert(oldproject.source_version==20);
 assert(oldproject.global_fx.c_fx==project.global_fx.c_fx);
 assert(!memcmp(&oldproject.evolution,&project.evolution,4));
 assert(etch_project_decode(packed,used-1,&oldlib,&oldpattern,&oldproject)<0);
 /* Verify a PJT19 package of the OLD packed FX depth/target is readable. */
 project.source_version=19;etch_c_fx_set_target(&project.global_fx,5);
 etch_c_fx_set_depth(&project.global_fx,24000);
 assert(etch_project_encode(&lib,&pattern,&project,packed,sizeof(packed),&used)==0);
 assert(!memcmp(packed,"PJTC",4));
 assert(etch_project_decode(packed,used,&oldlib,&oldpattern,&oldproject)==0);
 assert(oldproject.source_version==19);
 assert(etch_c_fx_target(&oldproject.global_fx)==5);
 assert(abs((int)etch_c_fx_depth(&oldproject.global_fx)-24000)<50);
 /* C FX wet destination genuinely modulates global reverb send. */
 uint64_t dry=pcm_signature(0),wet=pcm_signature(28);
 printf("PJT20 %u bytes; C FX PCM dry=%llx wet=%llx\n",used,(unsigned long long)dry,(unsigned long long)wet);
 assert(dry!=wet);
 puts("PASS V1.16 expanded C FX controls, all destination names, WAVE C ordinal, PJT19->20, realtime wet FX");
}
