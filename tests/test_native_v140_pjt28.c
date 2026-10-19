/* SPDX-License-Identifier: GPL-3.0-only. PJT28 velocity persistence and migration. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_wave_library wave,wave_out;
static struct etch_native_pattern pattern,pattern_out;
static struct etch_project_settings project,decoded;
static struct etch_preset_dsp_bank bank,outbank;
static uint8_t data[ETCH_PROJECT_MAX_BYTES];
static void roundtrip(uint8_t version){
 uint32_t used=0;
 project.source_version=version;
 assert(etch_project_encode(&wave,&pattern,&project,data,sizeof(data),&used)==0);
 assert(etch_project_decode(data,used,&wave_out,&pattern_out,&decoded)==0);
 assert(decoded.source_version==version);
 assert(outbank.slot[4].velocity_curve==(version==28?2u:0u));
 assert(outbank.slot[5].velocity_curve==(version==28?1u:0u));
 assert(outbank.slot[4].spectral_gain==(version==26?50u:67u));
 assert(etch_project_decode(data,used-1,&wave_out,&pattern_out,&decoded)!=0);
}
int main(void){
 etch_project_legacy(&wave,&pattern,&project);
 project.seq_length=16;project.seq_gate=75;project.tempo=120;
 etch_project_bind_preset_dsp(&project,&bank);
 etch_project_bind_preset_dsp(&decoded,&outbank);
 for(unsigned i=0;i<ETCH_PATCH_SLOTS;i++){
  bank.slot[i].spectral_part=2;
  bank.slot[i].spectral_gain=50;bank.slot[i].spectral_tilt=50;
  bank.slot[i].blend_a=i;bank.slot[i].blend_b=i;
 }
 bank.slot[4].velocity_curve=2;bank.slot[5].velocity_curve=1;
 bank.slot[4].spectral_gain=67;
 roundtrip(28);
 roundtrip(27);
 roundtrip(26);
 project.source_version=28;
 bank.slot[4].velocity_curve=3;
 uint32_t used;
 assert(etch_project_encode(&wave,&pattern,&project,data,sizeof(data),&used)!=0);
 puts("PASS PJT28 velocity curves, PJT27/PJT26 fallback, malformed curve validation");
}
