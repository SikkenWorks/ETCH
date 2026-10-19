/* SPDX-License-Identifier: GPL-3.0-only
 * Real native oscillator integration: main+two simultaneous group banks.
 * No browser DSP, no fake group routing: render actual C audio_core.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_tables.h"
static struct etch_native_audio s;
static struct etch_key_groups g;
static struct etch_native_patch patches[32];
static int16_t pcm[128];
static unsigned voice_for(unsigned note){
 for(unsigned i=0;i<8;i++)if(s.voice[i].active&&s.voice[i].note==note)return i;
 return 255u;
}
static long energy(int pitch){
 etch_native_midi_panic(&s);
 for(unsigned v=0;v<8;v++)s.voice[v].active=0;
 for(int j=0;j<12;j++)etch_native_render(&s,&(struct etch_native_controls){
 .morph=0,.depth=0,.gain=32767,.bank=&native_banks[0],.sustain_level=32767,
 .cutoff=16000,.env_enabled=1,.attack_step=0x7fff0000u,.release_step=0x7fff0000u},0,pcm,128);
 etch_native_midi(&s,0x90,(uint8_t)pitch,127);
 long sum=0;
 for(int j=0;j<12;j++){
  struct etch_native_controls c={0};
  c.morph=0;c.depth=0;c.gain=32767;c.bank=&native_banks[0];
  c.sustain_level=32767;c.cutoff=16000;c.env_enabled=1;
  c.attack_step=0x7fff0000u;c.release_step=0x7fff0000u;
  etch_native_render(&s,&c,0,pcm,128);
  if(j>=4)for(int i=0;i<128;i++)sum+=abs(pcm[i]);
 }
 return sum;
}
int main(void){
 etch_key_groups_init(&g);
 assert(etch_key_group_set(&g,0,4,36,47,1));
 assert(etch_key_group_set(&g,1,9,60,72,1));
 for(int k=0;k<32;k++){patches[k].valid=1;patches[k].sustain=32767;patches[k].attack_ms=1;patches[k].release_ms=10;}
 etch_native_groups_configure(&g);etch_native_groups_patch_bank(patches);
 etch_native_reset(&s);
 for(unsigned w=0;w<3;w++)for(unsigned l=0;l<6;l++)for(unsigned i=0;i<512;i++){
  native_banks[0].table[w][l][i]=etch_tables[0][l][i]; /* main: sine */
  native_banks[1].table[w][l][i]=0; /* group1: silent waveform */
  native_banks[2].table[w][l][i]=etch_tables[2][l][i]; /* group2: saw */
 }
 long main=energy(84),low=energy(40),high=energy(64);
 printf("Energy main=%ld group1=%ld group2=%ld\n",main,low,high);
 assert(main>1000 && low==0 && high>1000);
 /* Now verify that Group 1 can play its own non-silent waveform, too. */
 for(unsigned w=0;w<3;w++)for(unsigned l=0;l<6;l++)for(unsigned i=0;i<512;i++)
  native_banks[1].table[w][l][i]=etch_tables[1][l][i]; /* independent triangle */
 long group1_triangle=energy(40);
 printf("Group1 triangle energy=%ld\n",group1_triangle);
 assert(group1_triangle>1000);
 for(unsigned i=0;i<8;i++)s.voice[i].active=0;
 etch_native_midi(&s,0x90,40,100);
 etch_native_midi(&s,0x91,64,100);
 etch_native_midi(&s,0x92,84,100);
 assert(etch_native_voice_group(voice_for(168))==1);
 assert(etch_native_voice_group(voice_for(320))==2);
 assert(etch_native_voice_group(voice_for(468))==0);
 etch_native_midi(&s,0x80,40,0);
 assert(!s.voice[voice_for(168)].held);
 assert(s.voice[voice_for(320)].held);
 etch_native_groups_configure(0);etch_native_groups_patch_bank(0);
 puts("PASS three independent FM1 waveform banks and note ownership");
}
