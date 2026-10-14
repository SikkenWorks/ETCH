/* SPDX-License-Identifier: GPL-3.0-only. V1.28 legacy PJT24 / PJT25 roundtrip. */
#include "../firmware/fm1-native/native_project_codec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct etch_wave_library lib,loaded;
static struct etch_native_pattern pattern,decoded_pattern;
static struct etch_project_settings settings,readback;
static struct etch_riff2_bank riff,decoded;
static uint8_t bytes[ETCH_PROJECT_MAX_BYTES];
int main(void){
 uint32_t used;
 settings.source_version=25;settings.seq_length=16;settings.seq_gate=75;settings.tempo=120;
 etch_fx_defaults(&settings.global_fx);
 riff.riff_variants[0]=0x5987;riff.riff_mutations[0]=61;riff.riff_densities[0]=80;
 riff.riff_variants[3]=0x9376;riff.riff_mutations[3]=100;riff.riff_densities[3]=0;
 for(int i=0;i<32;i++)if(i!=0&&i!=3)riff.riff_densities[i]=50;
 etch_project_bind_riff2(&settings,&riff);
 assert(!etch_project_encode(&lib,&pattern,&settings,bytes,sizeof(bytes),&used));
 assert(bytes[0]=='P'&&bytes[1]=='J'&&bytes[2]=='T'&&bytes[3]=='I');
 etch_project_bind_riff2(&readback,&decoded);
 assert(!etch_project_decode(bytes,used,&loaded,&decoded_pattern,&readback));
 assert(readback.source_version==25);
 assert(!memcmp(&riff,&decoded,sizeof(riff)));
 etch_project_bind_riff2(&settings,&riff);
 settings.source_version=24;
 assert(!etch_project_encode(&lib,&pattern,&settings,bytes,sizeof(bytes),&used));
 memset(&decoded,0xff,sizeof(decoded));
 etch_project_bind_riff2(&readback,&decoded);
 assert(!etch_project_decode(bytes,used,&loaded,&decoded_pattern,&readback));
 assert(readback.source_version==24);
 for(int i=0;i<32;i++)assert(decoded.riff_densities[i]==50 && decoded.riff_mutations[i]==0 && decoded.riff_variants[i]==0);
 puts("PASS V1.28 PJT25 riff extras roundtrip and PJT24 compatibility");
}
