/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/native_tempo_restore.h"
#include "../firmware/fm1-native/native_project_codec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 struct etch_wave_library lib={0},outlib={0};
 struct etch_native_pattern pat={0},outpat={0};
 struct etch_project_settings cfg={0},loaded={0};
 static unsigned char encoded[ETCH_PROJECT_MAX_BYTES];
 uint32_t used=0;
 cfg.source_version=11;cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=173;
 etch_fx_defaults(&cfg.global_fx);
 /* Regression: old saved patch 120 vs newly saved project global BPM 173. */
 lib.reserved=1;lib.patches[0].valid=1;lib.patches[0].tempo=120;
 assert(etch_project_encode(&lib,&pat,&cfg,encoded,sizeof(encoded),&used)==0);
 assert(etch_project_decode(encoded,used,&outlib,&outpat,&loaded)==0);
 assert(loaded.tempo==173&&outlib.patches[0].tempo==120);
 assert(etch_saved_project_bpm(loaded.tempo,outlib.patches[0].tempo)==173);
 /* Existing patch loads keep their independent BPM; legacy/invalid values get fallback. */
 assert(etch_saved_project_bpm(0,115)==115);
 assert(etch_saved_project_bpm(0,0)==120);
 assert(etch_saved_project_bpm(240,120)==240);
 assert(etch_saved_project_bpm(30,120)==30);
 puts("V0.93 BPM project roundtrip, boot precedence, fallback PASS");
 return 0;
}
