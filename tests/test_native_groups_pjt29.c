/* PJT29: two user Groups are persistent without changing legacy projects. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_wave_library lib,decoded_lib;
static struct etch_native_pattern pattern,decoded_pattern;
static struct etch_project_settings project,decoded;
static struct etch_preset_dsp_bank dsp,decoded_dsp;
static uint8_t image[ETCH_PROJECT_MAX_BYTES];
static void save_load(uint8_t version){
 uint32_t used=0;project.source_version=version;
 assert(etch_project_encode(&lib,&pattern,&project,image,sizeof(image),&used)==0);
 assert(used>1000u);
 assert(etch_project_decode(image,used,&decoded_lib,&decoded_pattern,&decoded)==0);
 assert(decoded.source_version==version);
 if(version==29){
  assert(decoded_dsp.groups.group[0].enabled==1);
  assert(decoded_dsp.groups.group[0].preset==4);
  assert(decoded_dsp.groups.group[0].start==36);
  assert(decoded_dsp.groups.group[0].end==47);
  assert(decoded_dsp.groups.group[1].preset==9);
  assert(etch_key_group_route(&decoded_dsp.groups,60)==2);
  assert(etch_key_group_route(&decoded_dsp.groups,80)==0);
  uint32_t offset=used-8;
  image[offset+6]=90u;image[offset+7]=10u; /* invalid END <= START */
  assert(etch_project_decode(image,used,&decoded_lib,&decoded_pattern,&decoded)!=0);
 }else{
  assert(!decoded_dsp.groups.group[0].enabled);
  assert(!decoded_dsp.groups.group[1].enabled);
 }
 assert(etch_project_decode(image,used-1,&decoded_lib,&decoded_pattern,&decoded)!=0);
}
int main(void){
 etch_project_legacy(&lib,&pattern,&project);
 project.seq_length=16;project.seq_gate=75;project.tempo=120;
 etch_project_bind_preset_dsp(&project,&dsp);
 etch_project_bind_preset_dsp(&decoded,&decoded_dsp);
 for(unsigned i=0;i<ETCH_PATCH_SLOTS;i++){
  dsp.slot[i].spectral_part=2;dsp.slot[i].spectral_gain=50;
  dsp.slot[i].spectral_tilt=50;dsp.slot[i].blend_a=i;dsp.slot[i].blend_b=i;
 }
 etch_key_groups_init(&dsp.groups);
 assert(etch_key_group_set(&dsp.groups,0,4,36,47,1));
 assert(etch_key_group_set(&dsp.groups,1,9,60,72,1));
 save_load(29);
 save_load(28);
 save_load(27);
 puts("PASS PJT29 GROUPS roundtrip, PJT28/PJT27 migration, overlap/range rejection and truncated payload");
}
