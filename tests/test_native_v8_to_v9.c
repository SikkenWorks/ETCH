/* SPDX-License-Identifier: GPL-3.0-only: authentic V8 -> V9 compatibility. */
#include "../firmware/fm1-native/native_project_codec.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
static uint8_t bytes[ETCH_PROJECT_MAX_BYTES],new_bytes[ETCH_PROJECT_MAX_BYTES];
static struct etch_wave_library lib,readlib;
static struct etch_native_pattern pat,readpat;
static struct etch_project_settings cfg,readcfg;
int main(void){
 FILE *f=fopen("tests/fixtures/native-project-v8.bin","rb");assert(f);
 size_t n=fread(bytes,1,sizeof(bytes),f);assert(!ferror(f));fclose(f);
 assert(n>=200&&memcmp(bytes,"PJT8",4)==0);
 assert(!etch_project_decode(bytes,(uint32_t)n,&lib,&pat,&cfg));
 assert(cfg.source_version==8&&lib.slot[0][0]==-32768&&lib.slot[0][1]==32767&&pat.steps[1].hold_count==1);
 uint32_t used=0;assert(!etch_project_encode(&lib,&pat,&cfg,new_bytes,sizeof(new_bytes),&used));
 assert(memcmp(new_bytes,"PJT<",4)==0);
 assert(!etch_project_decode(new_bytes,used,&readlib,&readpat,&readcfg));
 assert(readcfg.source_version==12&&readlib.slot[0][0]==-32768&&readlib.slot[0][1]==32767);
 assert(!memcmp(&pat,&readpat,sizeof(pat)));
 assert(!memcmp(lib.wave_names,readlib.wave_names,sizeof(lib.wave_names)));
 puts("PASS authentic V8->V10 migration, waveform signed limits, per-trigger HOLD, patch and names retained");
}
