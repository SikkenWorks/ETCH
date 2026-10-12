/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_native_audio synth;
static struct etch_native_controls c;
static struct etch_wave_library lib,decoded_lib;
static struct etch_native_pattern pat,decoded_pat;
static struct etch_project_settings cfg,decoded_cfg;
static uint8_t packed[ETCH_PROJECT_MAX_BYTES];
static uint16_t seeds[ETCH_PATCH_SLOTS],read_seeds[ETCH_PATCH_SLOTS];
static int16_t out[128];
static uint64_t preview_sound(uint8_t preview,uint16_t morph,uint16_t depth,uint8_t insert){
 etch_native_reset(&synth);memset(&c,0,sizeof(c));
 etch_fx_defaults(&c.fx_settings);
 c.gain=32767;c.tempo=120;c.morph=morph;c.depth=depth;c.preview_wave=preview;
 c.sustain_level=32767;c.env_enabled=1;
 c.attack_step=0x7fff0000u;c.decay_step=0x7fff0000u;c.release_step=0x00300000u;
 if(insert){etch_pre_set_type(&c.fx_settings,0,0,3);etch_pre_set_depth(&c.fx_settings,0,0,24000);etch_pre_set_mix(&c.fx_settings,0,0,32767);}
 uint64_t h=1469598103934665603ull;
 for(int b=0;b<48;b++){
  etch_native_render(&synth,&c,1u,out,128);
  if(b<4)continue; /* let the 128-sample solo ramp fully settle */
  for(int i=0;i<128;i++)h=(h^(uint16_t)out[i])*1099511628211ull;
 }
 return h;
}
int main(void){
 struct etch_harmony_settings h={0,5,0,0}; /* C natural minor, KEYLOCK OFF */
 uint32_t seed=0x8925u,next=etch_riff_regenerate(seed);
 assert(next!=seed);
 uint32_t on=0,rests=0,changed=0;
 for(uint8_t i=0;i<16;i++){
  uint8_t a=etch_riff_pick(seed,i,60,&h,1),b=etch_riff_pick(next,i,60,&h,1);
  if(a==255)rests++;else{on++;static const uint8_t minor[7]={0,2,3,5,7,8,10};
   uint8_t good=0;for(int j=0;j<7;j++)if(a%12==minor[j])good=1;if(!good)fprintf(stderr,"FAIL scale step=%u pitch=%u pc=%u seed=%u\n",i,a,a%12u,(unsigned)seed);assert(good);}
  if(a!=b)changed++;
  assert(etch_riff_pick(seed,i,60,&h,1)==a);
 }
 assert(on>=8&&changed>=4&&rests<=8);
 assert(etch_riff_pick(seed,0,60,&h,1)!=etch_riff_pick(seed,0,60,&h,2)); /* octave control matters */
 uint64_t a=preview_sound(1,0,0,0);
 assert(a==preview_sound(1,32767,32000,0)); /* morph/C modulation isolated */
 assert(a!=preview_sound(2,0,0,0)); /* B is audibly distinct */
 assert(a!=preview_sound(1,0,0,1)); /* A's own insert is audible */
 assert(preview_sound(2,0,0,0)==preview_sound(2,0,0,1)); /* A insert does not leak into B preview */
 assert(a!=preview_sound(0,32767,0,0)); /* full synth still audible */
 etch_project_legacy(&lib,&pat,&cfg);
 cfg.source_version=21;cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;
 cfg.harmony=h;etch_fx_defaults(&cfg.global_fx);
 seeds[0]=0x1234u;seeds[9]=0xabceu;
 etch_project_bind_riffs(&cfg,seeds);
 uint32_t size=0;
 assert(etch_project_encode(&lib,&pat,&cfg,packed,sizeof(packed),&size)==0);
 assert(!memcmp(packed,"PJTE",4));
 etch_project_bind_riffs(&decoded_cfg,read_seeds);
 assert(etch_project_decode(packed,size,&decoded_lib,&decoded_pat,&decoded_cfg)==0);
 assert(decoded_cfg.source_version==21 && read_seeds[0]==seeds[0] && read_seeds[9]==seeds[9]);
 assert(etch_project_decode(packed,size-1,&decoded_lib,&decoded_pat,&decoded_cfg)!=0);
 /* Reading PJT20 must preserve old content but clear any stale riff seeds. */
 etch_project_bind_riffs(&cfg,seeds);cfg.source_version=20;
 assert(etch_project_encode(&lib,&pat,&cfg,packed,sizeof(packed),&size)==0);
 etch_project_bind_riffs(&decoded_cfg,read_seeds);
 assert(etch_project_decode(packed,size,&decoded_lib,&decoded_pat,&decoded_cfg)==0);
 assert(decoded_cfg.source_version==20 && read_seeds[0]==0 && read_seeds[9]==0);
 puts("PASS V1.18 scale riff, octave, regen, preview solo audio, PJT21 persistence, PJT20 migration");
}
