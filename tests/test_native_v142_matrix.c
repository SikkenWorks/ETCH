/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_advanced_controls_v130.h"
static struct etch_wave_library lib,loaded;
static struct etch_native_pattern pat,patout;
static struct etch_project_settings in,out;
static struct etch_preset_dsp_bank presets,restored;
static uint8_t bytes[ETCH_PROJECT_MAX_BYTES];
int main(void){
 struct etch_mod_matrix m={0};
 assert(etch_matrix_valid(&m)&&!etch_matrix_active(&m));
 m.route[0]=(struct etch_mod_route){5,0,100};
 assert(etch_matrix_apply_ex(&m,0,1000,100,0,0,0,0,64)==1000);
 assert(etch_matrix_apply_ex(&m,0,1000,100,0,0,0,127,64)>1000);
 m.route[0].amount=etch_matrix_encode_signed(-100);
 assert(m.route[0].amount==200&&etch_matrix_signed(m.route[0].amount)==-100);
 assert(etch_matrix_apply_ex(&m,0,10000,100,0,0,0,127,64)<10000);
 m.route[1]=(struct etch_mod_route){6,2,65};
 assert(etch_matrix_apply_ex(&m,2,1000,0,0,0,0,0,127)>1000);
 assert(etch_matrix_apply_ex(&m,2,1000,0,0,0,0,0,0)==1000);
 m.route[2]=(struct etch_mod_route){3,1,50};
 assert(etch_matrix_apply_ex(&m,1,11000,0,0,-32767,0,0,64)<11000);
 assert(etch_matrix_apply_ex(&m,1,11000,0,0,32767,0,0,64)>11000);
 assert(!etch_matrix_valid_legacy(&m));
 assert(sizeof(struct etch_mod_matrix)==12);
 uint8_t slot=0;m.route[3]=(struct etch_mod_route){1,1,0};
 etch_v130_matrix(&m,&slot,3,0,0,-1);
 assert(slot==3 && m.route[3].amount==101 && etch_matrix_signed(m.route[3].amount)==-1);
 etch_v130_matrix(&m,&slot,0,0,0,1);
 assert(m.route[3].amount==0);
 etch_project_legacy(&lib,&pat,&in);
 in.source_version=30;in.seq_length=16;in.seq_gate=75;in.tempo=120;
 in.matrix=m;
 etch_project_bind_preset_dsp(&in,&presets);
 etch_project_bind_preset_dsp(&out,&restored);
 presets.slot[4].matrix=m;
 etch_key_groups_init(&presets.groups);
 uint32_t used=0;
 assert(etch_project_encode(&lib,&pat,&in,bytes,sizeof(bytes),&used)==0);
 assert(!memcmp(bytes,"PJTN",4));
 assert(etch_project_decode(bytes,used,&loaded,&patout,&out)==0);
 assert(out.source_version==30);
 assert(!memcmp(&out.matrix,&m,sizeof m));
 assert(!memcmp(&restored.slot[4].matrix,&m,sizeof m));
 assert(etch_project_decode(bytes,used-1,&loaded,&patout,&out)!=0);
 in.source_version=29;
 assert(etch_project_encode(&lib,&pat,&in,bytes,sizeof(bytes),&used)!=0);
 in.matrix=(struct etch_mod_matrix){0};presets.slot[4].matrix=(struct etch_mod_matrix){0};
 assert(etch_project_encode(&lib,&pat,&in,bytes,sizeof(bytes),&used)==0);
 assert(!memcmp(bytes,"PJTM",4));
 assert(etch_project_decode(bytes,used,&loaded,&patout,&out)==0);
 assert(out.source_version==29&&!etch_matrix_active(&out.matrix));
 puts("PASS V1.42 signed routes, pressure, keytracking, 12-byte ABI, PJT29/30 roundtrip and corruption");
}
