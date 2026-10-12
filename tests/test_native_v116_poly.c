/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_poly_headroom.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct etch_native_audio s;
static struct etch_native_controls c;
static int16_t pcm[128*2];
static uint32_t hits,peaks,steps,frames;
static int32_t last;
static void run(uint32_t blocks,uint8_t is_stereo){
 for(uint32_t b=0;b<blocks;b++){
  if(is_stereo)etch_native_render_stereo(&s,&c,0,pcm,128);
  else etch_native_render(&s,&c,0,pcm,128);
  for(uint32_t j=0;j<128u*(is_stereo?2u:1u);j++){
   int32_t y=pcm[j];
   if(y>32760||y< -32760)hits++;
   if(y>peaks || -y>(int32_t)peaks)peaks=y<0?-y:y;
   if(frames && !is_stereo){int32_t d=y-last;if(d>25000||d< -25000)steps++;}
   last=y;frames++;
  }
 }
}
static void init(void){
 etch_native_reset(&s);memset(&c,0,sizeof(c));
 etch_fx_defaults(&c.fx_settings);c.gain=32767;c.tempo=120;c.morph=0;
 c.env_enabled=1;c.attack_step=0x7fff0000u;c.decay_step=0x7fff0000u;
 c.sustain_level=32767;c.release_step=0x00400000u;
 hits=peaks=steps=frames=0;last=0;
}
int main(void){
 /* The new attack gain reaches within <5% of the eight-voice target
  * before the original 192-sample note fade reaches full amplitude. */
 int32_t g=4096;
 for(int i=0;i<128;i++){g=etch_poly_headroom_step(g,8);assert(g<=8192 && g>=4096);}
 assert(g>=8100);
 for(int i=0;i<128;i++){int32_t next=etch_poly_headroom_step(g,4);assert(next<=g);g=next;}
 assert(g>7400); /* no release-driven pumping */
 for(int x=-70000;x<=70000;x+=37){
  int32_t y=etch_poly_safe_sample(x);
  assert(y<=31744&&y>=-31744);
  if(x>=-22000&&x<=22000)assert(x==y);
  if(x>0)assert(y>0);
 }
 assert(etch_poly_safe_sample(28000)==26500);
 assert(etch_poly_safe_sample(40000)==29500);
 for(int n=1;n<=8;n++){
  init();
  for(int i=0;i<n;i++)etch_native_midi(&s,0x90,(uint8_t)(60+i*2),127);
  run(36,0);
  assert(s.poly_div_q10>=4096 && s.poly_div_q10<=8192);
  assert(hits==0);
  printf("%d voices: max=%u clip=%u large_jumps=%u divisor=%d\n",n,peaks,hits,steps,s.poly_div_q10);
 }
 /* Drive, reverb/SPACE, morph mod and new Evolution active while
  * stealing voices, then release. Accept this only as host PCM testing;
  * physical USB/DAC underruns are not observable here. */
 init();c.drive=12000;c.reverb=24000;c.chorus=12000;
 c.evolution.depth=90;c.evolution.rate=60;c.evolution.mode=1;c.evolution.sync=0;
 c.filter_type=1;c.cutoff=6000;c.resonance=15000;
 for(int i=0;i<8;i++)etch_native_midi(&s,0x90,(uint8_t)(48+i*3),127);
 run(25,1);
 for(int i=0;i<48;i++){
  etch_native_midi(&s,0x90,(uint8_t)(48+(i*5)%52),127);
  run(1,1);
 }
 for(int i=0;i<52;i++)etch_native_midi(&s,0x80,(uint8_t)(48+i),0);
 run(64,1);
 assert(hits==0);
 printf("stress+FX+steal: peak=%u clips=%u steals=%u\n",peaks,hits,s.steal);
 assert(s.steal>0);
 puts("PASS V1.16 poly attack/release headroom, peak safety, mono/stereo 8-voice + FX + stealing");
}
