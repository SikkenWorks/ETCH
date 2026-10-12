/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_project_codec.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct etch_native_fx reverb;
static struct etch_native_audio synth;
static struct etch_native_controls c;
static int16_t dry[13000],mid[13000],wet[13000];
static struct etch_wave_library library,library2;
static struct etch_native_pattern notes,notes2;
static struct etch_project_settings project,project2;
static uint8_t packed[ETCH_PROJECT_MAX_BYTES];
static void render_fx(int16_t *dst,uint16_t mix){
 struct etch_fx_settings settings;etch_fx_defaults(&settings);etch_fx_reset(&reverb);
 for(unsigned n=0;n<13000;n++){
  /* Short, periodic carrier plus release: test sound within 3..30ms and tail. */
  int16_t input=(n<3300)?(int16_t)((n%91u)<45u?10000:-10000):0;
  int16_t r=0;
  etch_fx_process_stereo_ex(&reverb,input,0,0,mix,&settings,&dst[n],&r);
  if(!mix)assert(dst[n]==input && r==input);
 }
}
static void synth_render(uint8_t evolution,uint8_t mode,int16_t *dst){
 memset(&c,0,sizeof(c));etch_fx_defaults(&c.fx_settings);
 c.gain=32767;c.tempo=120;c.morph=8000;
 c.evolution.depth=evolution;c.evolution.rate=100;
 c.evolution.mode=mode;c.evolution.sync=2; /* one-shot note start */
 etch_native_reset(&synth);
 for(unsigned block=0;block<100;block++){
  int16_t pcm[128];etch_native_render(&synth,&c,1,pcm,128);
  memcpy(dst+block*128,pcm,128*sizeof(int16_t));
 }
}
int main(void){
 render_fx(dry,0);render_fx(mid,16384);render_fx(wet,32767);
 uint64_t diffmid=0,diffwet=0,early=0,tail=0;
 for(unsigned i=0;i<13000;i++){
  int32_t md=(int32_t)mid[i]-dry[i],wd=(int32_t)wet[i]-dry[i];
  diffmid+=md<0?-md:md;diffwet+=wd<0?-wd:wd;
  if(i>=200&&i<=1300)early+=wd<0?-wd:wd;
  if(i>=3300&&i<=9000)tail+=wet[i]<0?-wet[i]:wet[i];
 }
 printf("SPACE 0/50/100 PCM diff: %llu, %llu; early %llu tail %llu\n",
  (unsigned long long)diffmid,(unsigned long long)diffwet,
  (unsigned long long)early,(unsigned long long)tail);
 assert(diffmid>1000000 && diffwet>3000000 && early>50000 && tail>10000);
 assert(!etch_evolution_valid(&(struct etch_evolution_settings){101,10,0,0}));
 assert(!etch_evolution_valid(&(struct etch_evolution_settings){10,10,3,0}));
 assert(etch_evolution_shape(11000,0,0)==11000);
 assert(etch_evolution_shape(11000,16000,0)!=11000);
 assert(etch_evolution_shape(11000,32000,1)!=11000);
 assert(etch_evolution_triangle(0u)==0u);
 assert(etch_evolution_triangle(0x80000000u)>=32766u);
 int16_t *a=dry,*b=mid,*d=wet;
 synth_render(0,0,a);synth_render(100,0,b);synth_render(100,1,d);
 uint64_t fold_diff=0,sharp_diff=0;
 for(unsigned n=1500;n<12800;n++){
  int32_t fd=(int32_t)a[n]-b[n],sd=(int32_t)a[n]-d[n];
  fold_diff+=fd<0?-fd:fd;sharp_diff+=sd<0?-sd:sd;
 }
 printf("EVOLUTION fold %llu sharp %llu\n",(unsigned long long)fold_diff,(unsigned long long)sharp_diff);
 assert(fold_diff>500000 && sharp_diff>500000);
 etch_project_legacy(&library,&notes,&project);
 project.seq_length=16;project.tempo=120;project.source_version=19;
 project.evolution=(struct etch_evolution_settings){73,41,2,1};
 uint32_t used=0;
 assert(etch_project_encode(&library,&notes,&project,packed,sizeof(packed),&used)==0);
 assert(etch_project_decode(packed,used,&library2,&notes2,&project2)==0);
 assert(project2.source_version==19 && memcmp(&project2.evolution,&project.evolution,4)==0);
 assert(etch_project_decode(packed,used-1,&library2,&notes2,&project2)<0);
 project.source_version=18;
 assert(etch_project_encode(&library,&notes,&project,packed,sizeof(packed),&used)==0);
 assert(etch_project_decode(packed,used,&library2,&notes2,&project2)==0);
 assert(project2.source_version==18 && project2.evolution.depth==0);
 project.source_version=19;project.evolution.mode=255;
 assert(etch_project_encode(&library,&notes,&project,packed,sizeof(packed),&used)<0);
 puts("PASS V1.15 audible SPACE, evolving render, PJT19 roundtrip, PJT18 legacy, invalid rejection");
}
