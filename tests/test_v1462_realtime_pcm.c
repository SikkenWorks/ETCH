/* SPDX-License-Identifier: GPL-3.0-only
 * Deterministic golden PCM and host timing of block-cached mod-matrix paths.
 * This cannot substitute for physical FM1 deadline measurements. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_tables.h"
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_native_audio a;
static struct etch_native_controls c;
static struct etch_key_groups groups;
static struct etch_native_patch patches[ETCH_PATCH_SLOTS];
static struct etch_preset_dsp dsps[ETCH_PATCH_SLOTS];
static int16_t pcm[ETCH_NATIVE_FRAMES*2];
const struct etch_preset_dsp *etch_group_dsp_for_preset(uint8_t p){return p<ETCH_PATCH_SLOTS?&dsps[p]:0;}
static uint64_t hash_bytes(uint64_t h,const int16_t *buf,unsigned size){
 for(unsigned i=0;i<size;i++){uint16_t v=(uint16_t)buf[i];h=(h^(uint8_t)v)*1099511628211ull;h=(h^(uint8_t)(v>>8))*1099511628211ull;}
 return h;
}
static uint64_t now_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;}
static void setup(int scenario){
 etch_native_reset(&a);memset(&c,0,sizeof(c));etch_fx_defaults(&c.fx_settings);
 c.gain=32767;c.morph=11500;c.depth=6000;c.cutoff=12500;c.resonance=2000;c.filter_type=1;
 c.attack_step=0x7fff0000u;c.decay_step=0x10000u;c.release_step=0x10000u;
 c.sustain_level=27000;c.env_enabled=1;c.tempo=120;
 etch_native_groups_configure(0);etch_native_groups_patch_bank(0);
 if(scenario==1||scenario==3||scenario==5){
  c.matrix.route[0]=(struct etch_mod_route){1,0,82};
  c.matrix.route[1]=(struct etch_mod_route){2,2,173};
  c.matrix.route[2]=(struct etch_mod_route){5,1,85};
  c.matrix.route[3]=(struct etch_mod_route){6,3,102};
 }
 if(scenario==2||scenario==4||scenario==6){
  c.matrix.route[0]=(struct etch_mod_route){3,0,100};
  c.matrix.route[1]=(struct etch_mod_route){3,1,115};
  c.matrix.route[2]=(struct etch_mod_route){4,2,73};
  c.matrix.route[3]=(struct etch_mod_route){5,3,200};
  c.living.depth=55;c.living.color=60;
 }
 if(scenario>=3){
  etch_key_groups_init(&groups);
  etch_key_group_set(&groups,0,4,35,52,1);
  etch_key_group_set(&groups,1,9,59,76,1);
  memset(patches,0,sizeof(patches));memset(dsps,0,sizeof(dsps));
  patches[4].valid=1;patches[4].morph=1000;patches[4].depth=13000;patches[4].cutoff=18000;
  patches[4].sustain=30000;patches[4].attack_ms=2;patches[4].release_ms=220;
  patches[9].valid=1;patches[9].morph=27500;patches[9].depth=8200;patches[9].cutoff=9600;
  patches[9].sustain=30000;patches[9].attack_ms=5;patches[9].release_ms=280;
  dsps[4].matrix=c.matrix;
  dsps[9].matrix=c.matrix;
  /* Use exactly the current native factory mip tables in each shared bank. */
  for(int bank=0;bank<3;bank++)for(int w=0;w<3;w++)for(int lev=0;lev<6;lev++)
   memcpy(native_banks[bank].table[w][lev],etch_tables[w][lev],512u*sizeof(int16_t));
  etch_native_groups_configure(&groups);etch_native_groups_patch_bank(patches);
  c.bank=&native_banks[0];
 }
 if(scenario==5||scenario==6){
  c.chorus=6000;c.delay=9000;c.reverb=15000;
  c.fx_settings.chorus_rate=4000;c.fx_settings.delay_time=8000;
 }
 for(int i=0;i<8;i++){
  uint8_t note=(uint8_t)(35+i*7);
  etch_native_midi(&a,(uint8_t)(0x90u+(i%3)),note,(uint8_t)(75+i*6));
 }
 etch_native_midi(&a,0xd0,90,0);
}
int main(void){
 for(int sc=0;sc<7;sc++){
  setup(sc);
  uint64_t h=1469598103934665603ull;
  uint64_t ts=now_ns();
  for(int block=0;block<160;block++){
   if(block==60){etch_native_midi(&a,0xd1,115,0);etch_native_midi(&a,0xd2,30,0);}
   if(block==95){etch_native_midi(&a,0x80,35,0);etch_native_midi(&a,0x81,42,0);}
   etch_native_render_stereo(&a,&c,0,pcm,ETCH_NATIVE_FRAMES);
   h=hash_bytes(h,pcm,ETCH_NATIVE_FRAMES*2);
  }
  printf("scenario=%d pcm=%016llx host_ns=%llu\n",sc,(unsigned long long)h,(unsigned long long)(now_ns()-ts));
 }
 return 0;
}
