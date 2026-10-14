/* SPDX-License-Identifier: GPL-3.0-only
 * Host audio load regression. No claim to measure pi32v2 DMA/USB timing.
 */
#define _POSIX_C_SOURCE 200809L
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_tables.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static struct etch_native_audio s;
static struct etch_native_controls c;
static int16_t out[ETCH_NATIVE_FRAMES*2];
static struct etch_native_bank bank_a,bank_b;
static void init_banks(void){
 for(int w=0;w<3;w++)for(int m=0;m<6;m++)for(int j=0;j<512;j++){
  int16_t a=etch_tables[w][m][j];
  bank_a.table[w][m][j]=a;
  bank_b.table[w][m][j]=(int16_t)(a*3/4);
 }
}
static uint64_t elapsed_ns(struct timespec a, struct timespec b){
 return (uint64_t)(b.tv_sec-a.tv_sec)*1000000000ULL+(uint64_t)(b.tv_nsec-a.tv_nsec);
}
static void init(int voices,int living,int fx){
 memset(&c,0,sizeof(c));etch_native_reset(&s);etch_fx_defaults(&c.fx_settings);
 c.gain=27000;c.tempo=120;c.env_enabled=1;
 c.attack_step=0x7fff0000u;c.decay_step=0x7fff0000u;
 c.sustain_level=27000;c.release_step=0x00400000u;
 c.morph=15000;c.depth=9500;c.mod_target=1;c.mod_ratio=3;
 c.filter_type=fx?1:0;c.cutoff=fx?7000:0;c.resonance=fx?10000:0;
 c.living.depth=living?95u:0u;c.living.rate=95u;c.living.color=10u;c.living.vary=70u;
 c.organic.drift=fx?70:0;c.organic.rate=50;c.organic.phase=20;c.organic.variance=fx?35:0;
 c.chorus=fx?8000:0;c.delay=fx?8000:0;c.reverb=fx?15000:0;
 for(int i=0;i<voices;i++)etch_native_midi(&s,0x90,(uint8_t)(45+i*4),125);
}
int main(int argc,char **argv){
 init_banks();
 int blocks=argc>1?atoi(argv[1]):200;
 if(blocks<1||blocks>3000)return 2;
 for(int mode=0;mode<4;mode++)for(int n=1;n<=8;n=n==1?4:n==4?8:9){
  int living=mode!=0,fx=mode>=2;
  init(n,living,fx);
  if(mode==3)c.bank=&bank_a;
  struct timespec t0,t1;clock_gettime(CLOCK_MONOTONIC,&t0);
  uint32_t crc=2166136261u;uint32_t maxabs=0u;
  for(int k=0;k<blocks;k++){
   if(mode>=2 && k%12==0){c.cutoff=(uint16_t)(1000+(k%10)*650);}
   if(mode==3 && k%8==0)c.bank=(c.bank==&bank_a)?&bank_b:&bank_a;
   etch_native_render_stereo(&s,&c,0,out,ETCH_NATIVE_FRAMES);
   for(int j=0;j<ETCH_NATIVE_FRAMES*2;j++){
    int32_t x=out[j];if(x<0)x=-x;if((uint32_t)x>maxabs)maxabs=(uint32_t)x;
    crc^=(uint16_t)out[j];crc*=16777619u;
   }
  }
  clock_gettime(CLOCK_MONOTONIC,&t1);
  assert(s.voice_limit==8u);assert(maxabs<=32767u);
  printf("mode=%d voices=%d ns_per_frame=%.2f checksum=%08x peak=%u\n",
    mode,n,(double)elapsed_ns(t0,t1)/((uint64_t)blocks*ETCH_NATIVE_FRAMES),crc,maxabs);
 }
 puts("PASS host 1/4/8-voice sustained polyphonic stress incl. LIVING, Organic, FX, and repeated crossfades");
}
