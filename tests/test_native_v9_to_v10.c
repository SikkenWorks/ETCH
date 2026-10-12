/* Genuine PJT9 bytes serialized with ETCH V0.47's original codec. */
#include "../firmware/fm1-native/native_project_codec.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static uint8_t old[ETCH_PROJECT_MAX_BYTES],next[ETCH_PROJECT_MAX_BYTES];
static struct etch_wave_library waves,loaded;
static struct etch_native_pattern pat,round;
static struct etch_project_settings cfg,restored;
int main(void){
 FILE *f=fopen("tests/fixtures/native-project-v9.bin","rb");assert(f);
 size_t n=fread(old,1,sizeof(old),f);assert(!ferror(f));fclose(f);
 assert(n>100&&memcmp(old,"PJT9",4)==0);
 assert(etch_project_decode(old,(uint32_t)n,&waves,&pat,&cfg)==0&&cfg.source_version==9);
 assert(pat.steps[0].count==2&&pat.steps[1].notes[0].note==67);
 assert(cfg.patterns.used>0&&waves.wave_names[0][1]=='9');
 uint32_t used=0;assert(etch_project_encode(&waves,&pat,&cfg,next,sizeof(next),&used)==0);
 assert(memcmp(next,"PJT<",4)==0);
 assert(etch_project_decode(next,used,&loaded,&round,&restored)==0);
 assert(restored.source_version==12&&memcmp(&round,&pat,sizeof(pat))==0);
 assert(memcmp(waves.slot,loaded.slot,sizeof(waves.slot))==0);
 assert(memcmp(cfg.patterns.data,restored.patterns.data,cfg.patterns.used)==0);
 puts("PASS genuine V9 -> V10 codec migration, saved notes/patches/waves preserved");
}
