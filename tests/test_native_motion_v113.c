/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_wave_library original,loaded;
static struct etch_native_pattern sequence,decoded;
static struct etch_project_settings settings,restored;
static uint8_t packed[ETCH_PROJECT_MAX_BYTES];
int main(void){
 etch_project_legacy(&original,&sequence,&settings);
 settings.seq_length=16;settings.tempo=120;settings.seq_rate=2;
 settings.source_version=18;
 etch_motion_clear(&settings.motion);
 assert(etch_motion_valid(&settings.motion));
 for(uint32_t i=0;i<64u;i++){
  uint16_t v[4]={(uint16_t)(i*511u),(uint16_t)((63-i)*511u),
                  (uint16_t)(5000u+i*170u),0u};
  etch_motion_capture(&settings.motion,v,i==0?0u:7u);
 }
 assert(settings.motion.length==64u && settings.motion.mask==7u);
 assert(etch_motion_at(&settings.motion,0u,0u)==0u);
 assert(etch_motion_at(&settings.motion,0u,4410u)==511u);
 assert(etch_motion_at(&settings.motion,0u,2205u)>200u);
 assert(etch_motion_at(&settings.motion,0u,64u*4410u)==0u);
 assert(etch_motion_at(&settings.motion,0u,63u*4410u+2205u)<settings.motion.lane[0][63]);
 uint32_t used=0;assert(etch_project_encode(&original,&sequence,&settings,packed,sizeof(packed),&used)==0);
 assert(used>500u && etch_project_decode(packed,used,&loaded,&decoded,&restored)==0);
 assert(restored.source_version==18u && restored.motion.length==64u && restored.motion.mask==7u);
 for(uint32_t k=0;k<4u;k++)for(uint32_t i=0;i<64u;i++)assert(restored.motion.lane[k][i]==settings.motion.lane[k][i]);
 assert(etch_project_decode(packed,used-1u,&loaded,&decoded,&restored)<0);
 /* Legacy PJT17 codec is unchanged and old projects load with MOTION OFF. */
 settings.source_version=17u;
 assert(etch_project_encode(&original,&sequence,&settings,packed,sizeof(packed),&used)==0);
 assert(etch_project_decode(packed,used,&loaded,&decoded,&restored)==0);
 assert(restored.source_version==17u && restored.motion.length==0u && restored.motion.mask==0u);
 /* Malformed clips must never be written. */
 settings.source_version=18u;settings.motion.lane[0][0]=40000u;
 assert(etch_project_encode(&original,&sequence,&settings,packed,sizeof(packed),&used)<0);
 puts("PASS V1.13 motion interpolation, PJT18 roundtrip, legacy migration and corrupt rejection");
 return 0;
}
