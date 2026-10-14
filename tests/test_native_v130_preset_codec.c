/* SPDX-License-Identifier: GPL-3.0-only
 * PJT26 independent presets, checksum/length, PJT25 backward reading.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_wave_library lib,loaded;
static struct etch_native_pattern pattern,loaded_pattern;
static struct etch_project_settings cfg,decoded;
static struct etch_preset_dsp_bank bank,bank2;
static uint8_t stream[ETCH_PROJECT_MAX_BYTES];
int main(void){
 etch_project_legacy(&lib,&pattern,&cfg);
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;cfg.source_version=26;
 etch_project_bind_preset_dsp(&cfg,&bank);
 etch_project_bind_preset_dsp(&decoded,&bank2);
 bank.slot[2].organic=(struct etch_organic_settings){77,90,15,61};
 bank.slot[2].living=(struct etch_living_settings){66,22,94,11};
 bank.slot[2].evolution=(struct etch_evolution_settings){0};
 bank.slot[2].matrix.route[0]=(struct etch_mod_route){2,1,45};
 bank.slot[2].spectral_wave=2;bank.slot[2].spectral_part=17;
 bank.slot[2].blend_a=1;bank.slot[2].blend_b=3;
 bank.slot[2].blend_pct=49;bank.slot[2].blend_on=0;
 bank.slot[3].organic=(struct etch_organic_settings){1,2,3,4};
 bank.slot[3].living=(struct etch_living_settings){8,9,10,11};
 uint32_t used=0;
 assert(!etch_project_encode(&lib,&pattern,&cfg,stream,sizeof(stream),&used));
 assert(!memcmp(stream,"PJTJ",4));
 assert(!etch_project_decode(stream,used,&loaded,&loaded_pattern,&decoded));
 assert(decoded.source_version==26);
 for(unsigned i=0;i<ETCH_PATCH_SLOTS;i++){
  assert(!memcmp(&bank.slot[i],&bank2.slot[i],30u));
  assert(bank2.slot[i].spectral_gain==50 && bank2.slot[i].spectral_tilt==50);
 }
 assert(bank2.slot[2].organic.drift==77&&bank2.slot[3].organic.drift==1);
 assert(etch_project_decode(stream,used-1,&loaded,&loaded_pattern,&decoded));
 cfg.source_version=25;
 assert(!etch_project_encode(&lib,&pattern,&cfg,stream,sizeof(stream),&used));
 assert(!etch_project_decode(stream,used,&loaded,&loaded_pattern,&decoded));
 assert(decoded.source_version==25);
 puts("PASS PJT26: independent preset DSP + edit state, 32 slots, PJT25 backward read");
}
