/* SPDX-License-Identifier: GPL-3.0-only
 * PJT27 SPECTRAL independent editor registers and compatibility with PJT26.
 */
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_wave_library original,loaded;
static struct etch_native_pattern pattern,loaded_pattern;
static struct etch_project_settings cfg,decoded;
static struct etch_preset_dsp_bank slots,restored;
static uint8_t data[ETCH_PROJECT_MAX_BYTES];
int main(void){
 etch_project_legacy(&original,&pattern,&cfg);
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;cfg.source_version=27;
 etch_project_bind_preset_dsp(&cfg,&slots);etch_project_bind_preset_dsp(&decoded,&restored);
 for(int i=0;i<ETCH_PATCH_SLOTS;i++){
  slots.slot[i].spectral_gain=50;slots.slot[i].spectral_tilt=50;
  slots.slot[i].spectral_part=2;
  slots.slot[i].blend_a=(uint8_t)i;slots.slot[i].blend_b=(uint8_t)i;
 }
 slots.slot[2].spectral_gain=64;slots.slot[2].spectral_tilt=10;
 slots.slot[3].spectral_gain=22;slots.slot[3].spectral_tilt=98;
 slots.slot[2].living=(struct etch_living_settings){90,80,20,17};
 uint32_t size=0;
 assert(etch_project_encode(&original,&pattern,&cfg,data,sizeof(data),&size)==0);
 assert(memcmp(data,"PJTK",4)==0);
 assert(etch_project_decode(data,size,&loaded,&loaded_pattern,&decoded)==0);
 assert(decoded.source_version==27);
 assert(!memcmp(&slots,&restored,sizeof(slots)));
 assert(restored.slot[3].spectral_gain==22 && restored.slot[3].spectral_tilt==98);
 assert(etch_project_decode(data,size-1,&loaded,&loaded_pattern,&decoded)!=0);
 /* Real old snapshot contains exactly 30 bytes per DSP slot, not 32. */
 cfg.source_version=26;
 assert(etch_project_encode(&original,&pattern,&cfg,data,sizeof(data),&size)==0);
 assert(memcmp(data,"PJTJ",4)==0);
 assert(etch_project_decode(data,size,&loaded,&loaded_pattern,&decoded)==0);
 assert(decoded.source_version==26);
 assert(restored.slot[2].spectral_gain==50 && restored.slot[2].spectral_tilt==50);
 assert(restored.slot[2].living.depth==90);
 cfg.source_version=27;slots.slot[3].spectral_tilt=101;
 assert(etch_project_encode(&original,&pattern,&cfg,data,sizeof(data),&size)!=0);
 puts("PASS PJT27/PJT26 independent spectral controls, persistence, validation, backward reading");
 return 0;
}
