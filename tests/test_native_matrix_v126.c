/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.26 MOD MATRIX: bounded routing, PJT24, backward-compatible bypass.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_native_audio audio;
static struct etch_native_controls control;
static struct etch_wave_library lib,loaded_lib;
static struct etch_native_pattern pattern,loaded_pattern;
static struct etch_project_settings settings,loaded_settings;
static uint8_t bytes[ETCH_PROJECT_MAX_BYTES];
static int16_t pcm[128];
static uint64_t hash(struct etch_mod_matrix matrix){
 etch_native_reset(&audio);memset(&control,0,sizeof(control));etch_fx_defaults(&control.fx_settings);
 control.gain=32767;control.morph=12000;control.depth=0;control.cutoff=7000;control.filter_type=1;
 control.env_enabled=1;control.attack_step=0x7fff0000u;control.decay_step=0x7fff0000u;
 control.sustain_level=32767;control.release_step=0x10000u;control.tempo=120;
 control.matrix=matrix;
 etch_native_midi(&audio,0x90,60,127);etch_native_midi(&audio,0x90,67,79);
 uint64_t h=1469598103934665603ull;
 for(int k=0;k<28;k++){
  etch_native_render(&audio,&control,0,pcm,128);
  for(int j=0;j<128;j++)h=(h^(uint16_t)pcm[j])*1099511628211ull;
 }
 return h;
}
int main(void){
 struct etch_mod_matrix zero={0};
 assert(etch_matrix_valid(&zero)&&!etch_matrix_active(&zero));
 assert(etch_matrix_apply(&zero,0,12000,127,32767,32767,32767)==12000);
 uint64_t baseline=hash(zero);
 struct etch_mod_matrix m={0};
 m.route[0]=(struct etch_mod_route){1,0,100};
 assert(etch_matrix_active(&m));
 assert(etch_matrix_apply(&m,0,1000,127,0,0,0)>1000);
 assert(etch_matrix_apply(&m,0,1000,0,0,0,0)==1000);
 assert(etch_matrix_apply(&m,1,1000,127,0,0,0)==1000);
 assert(hash(m)!=baseline);
 m.route[0]=(struct etch_mod_route){3,0,100};
 assert(etch_matrix_apply(&m,0,16000,127,0,-32767,0)<16000);
 assert(etch_matrix_apply(&m,0,16000,127,0,32767,0)>16000);
 m.route[1]=(struct etch_mod_route){2,2,100};
 m.route[2]=(struct etch_mod_route){4,3,100};
 m.route[3]=(struct etch_mod_route){1,1,100};
 assert(etch_matrix_valid(&m));
 assert(etch_matrix_apply(&m,2,32000,127,32767,0,0)==32767); // saturated
 assert(etch_matrix_apply(&m,3,32767,127,0,0,32767)==32767);
 assert(hash(m)!=baseline);
 m.route[1].amount=101;assert(etch_matrix_valid(&m)&&!etch_matrix_valid_legacy(&m));m.route[1].amount=100;
 m.route[2].source=5;assert(etch_matrix_valid(&m)&&!etch_matrix_valid_legacy(&m));m.route[2].source=4;
 etch_project_legacy(&lib,&pattern,&settings);settings.source_version=24;
 settings.seq_length=16;settings.seq_gate=75;settings.tempo=120;
 settings.organic=(struct etch_organic_settings){10,20,30,40};
 settings.living=(struct etch_living_settings){55,40,30,20};
 settings.matrix=m;
 uint32_t used=0;
 assert(!etch_project_encode(&lib,&pattern,&settings,bytes,sizeof(bytes),&used));
 assert(!memcmp(bytes,"PJTH",4));
 assert(!etch_project_decode(bytes,used,&loaded_lib,&loaded_pattern,&loaded_settings));
 assert(loaded_settings.source_version==24);
 assert(!memcmp(&settings.matrix,&loaded_settings.matrix,sizeof(m)));
 assert(!memcmp(&settings.living,&loaded_settings.living,sizeof(settings.living)));
 assert(etch_project_decode(bytes,used-1,&loaded_lib,&loaded_pattern,&loaded_settings));
 // Corrupted saved mod routing fails closed.
 bytes[used-3]=5u;
 assert(etch_project_decode(bytes,used,&loaded_lib,&loaded_pattern,&loaded_settings));
 bytes[used-3]=m.route[3].source;
 settings.source_version=23;
 assert(!etch_project_encode(&lib,&pattern,&settings,bytes,sizeof(bytes),&used));
 assert(!etch_project_decode(bytes,used,&loaded_lib,&loaded_pattern,&loaded_settings));
 assert(loaded_settings.source_version==23&&!etch_matrix_active(&loaded_settings.matrix));
 assert(loaded_settings.matrix.route[0].amount==0);
 assert(baseline==hash(zero));
 puts("PASS V1.26 MOD MATRIX: per-voice audio, source/destination routing, saturation, PJT24/PJT23, zero bypass");
 return 0;
}
