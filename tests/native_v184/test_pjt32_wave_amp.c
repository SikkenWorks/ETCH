/* SPDX-License-Identifier: GPL-3.0-only. PJT32 backwards-readable, same 33B DSP record. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "native_project_codec.h"
#include "native_wave_amp.h"
static struct etch_wave_library wave,wave_out;
static struct etch_native_pattern pat,pat_out;
static struct etch_project_settings project,decoded;
static struct etch_preset_dsp_bank bank,outbank;
static uint8_t data[ETCH_PROJECT_MAX_BYTES];
static void basic(uint8_t v){
 uint32_t used=0;project.source_version=v;
 assert(etch_project_encode(&wave,&pat,&project,data,sizeof(data),&used)==0);
 assert(used>200);
 assert(etch_project_decode(data,used,&wave_out,&pat_out,&decoded)==0);
 assert(decoded.source_version==v);
 assert(etch_project_decode(data,used-1u,&wave_out,&pat_out,&decoded)!=0);
}
int main(void){
 etch_project_legacy(&wave,&pat,&project);
 etch_project_bind_preset_dsp(&project,&bank);
 etch_project_bind_preset_dsp(&decoded,&outbank);
 project.seq_length=16;project.seq_gate=75;project.tempo=120;
 for(unsigned i=0;i<32;i++){
  bank.slot[i].spectral_part=2;bank.slot[i].spectral_gain=50;
  bank.slot[i].spectral_tilt=50;bank.slot[i].blend_a=i;bank.slot[i].blend_b=i;
 }
 etch_key_groups_init(&bank.groups);etch_perf_init(&bank.performance);
 bank.slot[2].spectral_wave=1;
 bank.slot[2].velocity_curve=2;
 basic(31);
 assert(outbank.slot[2].spectral_wave==1 && outbank.slot[2].velocity_curve==2);
 bank.slot[2].spectral_wave=etch_wamp_pack_wave(1,3); /* selected B? plus morph gain */
 bank.slot[2].velocity_curve=etch_wamp_pack_velocity(2,63);
 bank.slot[3].spectral_wave=etch_wamp_pack_wave(2,1);
 bank.slot[3].velocity_curve=etch_wamp_pack_velocity(1,32);
 basic(32);
 assert(outbank.slot[2].spectral_wave==bank.slot[2].spectral_wave);
 assert(outbank.slot[2].velocity_curve==bank.slot[2].velocity_curve);
 assert(etch_wamp_mode(outbank.slot[2].spectral_wave)==3);
 assert(etch_wamp_depth6(outbank.slot[2].velocity_curve)==63);
 assert(etch_wamp_spectral(outbank.slot[2].spectral_wave)==1);
 assert(etch_wamp_velocity(outbank.slot[2].velocity_curve)==2);
 assert(etch_wamp_mode(outbank.slot[3].spectral_wave)==1);
 assert(etch_wamp_depth6(outbank.slot[3].velocity_curve)==32);
 assert(sizeof(struct etch_preset_dsp)==33u);
 /* Bad reserved upper nibble is rejected before writes. */
 bank.slot[2].spectral_wave|=0x80u;
 uint32_t used=0;
 assert(etch_project_encode(&wave,&pat,&project,data,sizeof(data),&used)!=0);
 bank.slot[2].spectral_wave&=0x7fu;
 /* A PJT31 packet cannot pretend to contain PJT32 bits. */
 project.source_version=31;
 assert(etch_project_encode(&wave,&pat,&project,data,sizeof(data),&used)!=0);
 puts("PASS PJT32: old project read, new per-preset WAVE AMP encode/decode, strict validation, 33B unchanged");
}
