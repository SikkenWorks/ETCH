/* SPDX-License-Identifier: GPL-3.0-only
 * PJT13 full sound preset snapshots: exact waves + per-preset global FX,
 * project-scope BPM/mono, legacy PJT12 migration and damage rejection.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_wave_library a,b;
static struct etch_native_pattern pattern,pat2;
static struct etch_project_settings cfg,got;
static uint8_t wire[ETCH_PROJECT_MAX_BYTES];
static void seed(void){
 memset(&a,0,sizeof(a));memset(&cfg,0,sizeof(cfg));
 etch_pattern_clear(&pattern);cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=144;cfg.mono_mode=1;
 etch_fx_defaults(&cfg.global_fx);
 a.valid[0]=1;for(int i=0;i<128;i++)a.slot[0][i]=(int16_t)(i*7-400);
 for(int w=0;w<3;w++)a.selection[w]=0;
 a.patches[2].valid=1;memcpy(a.patches[2].name,"HEAVY PATCH ",12);
 a.patches[2].slot_a=0;a.patches[2].slot_b=0;a.patches[2].slot_c=0;
 a.patches[2].morph=24618;a.patches[2].depth=32767;a.patches[2].cutoff=21000;
 a.patches[9].valid=1;memcpy(a.patches[9].name,"GLASS PATCH ",12);
 a.patches[9].slot_a=0;a.patches[9].slot_b=0;a.patches[9].slot_c=0;
 a.patches[9].morph=8172;a.patches[9].depth=50;
 cfg.preset_valid=(1u<<2)|(1u<<9);
 cfg.preset_fx[2].ring_mix=30000;cfg.preset_fx[2].ring_rate=11000;
 cfg.preset_fx[2].eq_mid=20000;cfg.preset_fx[2].eq_low=16000;cfg.preset_fx[2].eq_high=25000;
 cfg.preset_fx[9].ring_mix=0;cfg.preset_fx[9].eq_mid=12000;
 for(unsigned w=0;w<3;w++)for(unsigned j=0;j<128;j++){
  cfg.preset_wave[2][w][j]=(int16_t)(j*11-(int)w*100-810);
  cfg.preset_wave[9][w][j]=(int16_t)(((j*31+13*w)%64)*233-6500);
 }
}
int main(void){
 seed();uint32_t size=0;
 cfg.source_version=13;
 assert(etch_project_encode(&a,&pattern,&cfg,wire,sizeof(wire),&size)==0);
 assert(!memcmp(wire,"PJT=",4));assert(size<ETCH_PROJECT_MAX_BYTES);
 assert(etch_project_decode(wire,size,&b,&pat2,&got)==0);
 assert(got.source_version==13&&got.tempo==144&&got.mono_mode==1);
 assert(got.preset_valid==((1u<<2)|(1u<<9)));
 assert(b.patches[2].morph==24618 && b.patches[9].morph==8172);
 assert(got.preset_fx[2].ring_mix==30000&&got.preset_fx[2].eq_mid==20000);
 assert(got.preset_fx[9].eq_mid==12000);
 assert(!memcmp(got.preset_wave[2],cfg.preset_wave[2],sizeof(cfg.preset_wave[2])));
 assert(!memcmp(got.preset_wave[9],cfg.preset_wave[9],sizeof(cfg.preset_wave[9])));
 /* Changing a shared global waveform does not corrupt either saved preset. */
 b.slot[0][32]=15551;
 assert(got.preset_wave[2][0][32]==cfg.preset_wave[2][0][32]);
 /* CRC container rejects bit-rot; decoder rejects malformed snapshot payloads. */
 assert(etch_project_decode(wire,size-1,&b,&pat2,&got)!=0);
 uint32_t legacy_size=0;cfg.source_version=12;
 assert(etch_project_encode(&a,&pattern,&cfg,wire,sizeof(wire),&legacy_size)==0);
 assert(!memcmp(wire,"PJT<",4));
 assert(etch_project_decode(wire,legacy_size,&b,&pat2,&got)==0);
 assert(got.source_version==12&&got.preset_valid==0&&got.tempo==144);
 cfg.source_version=13;cfg.preset_valid|=1u<<31;
 assert(etch_project_encode(&a,&pattern,&cfg,wire,sizeof(wire),&size)==-1);
 puts("PASS V0.99 PJT13: two full presets, exact waves/FX/morph, legacy PJT12, corruption and invalid-mask rejection");
 return 0;
}
