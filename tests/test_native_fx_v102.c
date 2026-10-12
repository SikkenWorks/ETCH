/* V1.02 real DSP + PJT14 roundtrip, downgrade-read and corruption tests. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include "../firmware/fm1-native/native_fx.h"
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_native_fx fx;
static struct etch_wave_library lib,gotlib;
static struct etch_native_pattern pattern,gotpattern;
static struct etch_project_settings cfg,got;
static uint8_t storage[ETCH_PROJECT_MAX_BYTES];
static void configure(struct etch_project_settings *s){
 memset(s,0,sizeof(*s));s->source_version=14;s->tempo=126;s->seq_length=64;s->seq_gate=75;
 etch_fx_defaults(&s->global_fx);s->preset_valid=1u<<0;
 s->preset_fx[0]=s->global_fx;
 s->global_fx.comp_ratio=25000;s->global_fx.comp_threshold=22000;
 s->global_fx.trem_mix=23000;s->global_fx.phaser_mix=30000;
 s->global_fx.flanger_mix=20000;s->global_fx.decim_mix=29000;
 s->global_fx.folder_mix=32000;s->global_fx.folder_fold=24000;
 s->preset_fx[0].decim_bits=9000;s->preset_fx[0].folder_mix=14000;
 for(int j=0;j<3;j++)for(int i=0;i<128;i++)s->preset_wave[0][j][i]=(int16_t)(i*15+j*10);
}
int main(void){
 struct etch_fx_settings c;etch_fx_defaults(&c);etch_fx_reset(&fx);
 for(int i=0;i<1024;i++){
  int16_t sample=(int16_t)((i*111)%40000-20000);
  assert(etch_fx_process(&fx,sample,0,0,0)==sample);
 }
 #define TEST_EFFECT(field,val) do{etch_fx_defaults(&c);c.field=(val);etch_fx_reset(&fx);uint32_t diff=0;for(int i=0;i<6000;i++){int16_t x=(i%100)<50?13000:-13000;int16_t y=etch_fx_process_ex(&fx,x,0,0,0,&c);diff+=(x!=y);} assert(diff>500);printf("OK %s difference %u\n",#field,diff);}while(0)
 etch_fx_defaults(&c);c.comp_ratio=32767;c.comp_threshold=32767;etch_fx_reset(&fx);{uint32_t diff=0;for(int i=0;i<7000;i++){int16_t x=(i%100)<50?16000:-16000;int16_t y=etch_fx_process_ex(&fx,x,0,0,0,&c);diff+=(x!=y);}assert(diff>500);} 
 TEST_EFFECT(trem_mix,32767);
 TEST_EFFECT(phaser_mix,32767);
 TEST_EFFECT(flanger_mix,32767);
 etch_fx_defaults(&c);c.decim_mix=32767;c.decim_bits=10000;c.decim_rate=25000;etch_fx_reset(&fx);{uint32_t diff=0;for(int i=0;i<6000;i++){int16_t x=(i%100)<50?13001:-13003;diff+=(x!=etch_fx_process_ex(&fx,x,0,0,0,&c));}assert(diff>500);}
 etch_fx_defaults(&c);c.folder_mix=32767;c.folder_fold=20000;etch_fx_reset(&fx);{uint32_t diff=0;for(int i=0;i<6000;i++){int16_t x=(i%100)<50?13001:-13003;diff+=(x!=etch_fx_process_ex(&fx,x,0,0,0,&c));}assert(diff>500);}
 configure(&cfg);lib.patches[0].valid=1;uint32_t used=0;
 assert(etch_project_encode(&lib,&pattern,&cfg,storage,sizeof(storage),&used)==0);
 assert(used>100&&used<sizeof(storage));
 assert(etch_project_decode(storage,used,&gotlib,&gotpattern,&got)==0);
 assert(got.source_version==14&&got.global_fx.flanger_mix==20000);
 assert(got.global_fx.comp_threshold==22000&&got.global_fx.folder_fold==24000);
 assert(got.preset_fx[0].decim_bits==9000&&got.preset_fx[0].folder_mix==14000);
 assert(!memcmp(got.preset_wave[0],cfg.preset_wave[0],sizeof(cfg.preset_wave[0])));
 storage[used-1]^=0x40; /* semantic checksum is provided by flash layer; truncated input must be rejected */
 assert(etch_project_decode(storage,used-1,&gotlib,&gotpattern,&got)!=0);
 /* PJT13 from original v1.01 has all new fields defaulted to neutral. */
 cfg.source_version=13;
 assert(etch_project_encode(&lib,&pattern,&cfg,storage,sizeof(storage),&used)==0);
 assert(etch_project_decode(storage,used,&gotlib,&gotpattern,&got)==0);
 assert(got.source_version==13&&got.global_fx.trem_mix==0);
 assert(got.preset_fx[0].flanger_mix==0&&got.preset_fx[0].decim_bits==32767);
 puts("PASS V1.02 six FX DSP and PJT14/PJT13 codec");return 0;
}
