/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/native_performance_midi.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
static uint32_t starts,stops,clocks;
static int emit(uint32_t packet,void *ctx){(void)ctx;
 uint8_t status=(uint8_t)(packet>>8);
 if(status==0xf8)clocks++;
 if(status==0xfa)starts++;
 if(status==0xfc)stops++;
 return 1;
}
static void advance(struct etch_perf_midi *m, struct etch_native_audio *a,
 struct etch_native_controls *c,uint32_t frames){
 while(frames){uint32_t block=frames>128?128:frames;
  etch_perf_midi_update(m,a,c,1,block,emit,0);frames-=block;
 }
}
int main(void){
 struct etch_perf_midi m;struct etch_native_audio a;struct etch_native_controls c;
 memset(&a,0,sizeof a);memset(&c,0,sizeof c);
 const uint8_t tempi[]={30,89,121,137,240};
 for(unsigned j=0;j<sizeof(tempi);j++){
  etch_perf_midi_reset(&m);starts=stops=clocks=0;
  c.tempo=tempi[j];c.seq_run=1;c.clock_follow=0;
  advance(&m,&a,&c,44100u*60u); /* exact 60 seconds */
  assert(starts==1&&stops==0);
  assert(clocks==((uint32_t)tempi[j]*24u));
  c.clock_follow=1;
  advance(&m,&a,&c,44100u);assert(stops==1&&clocks==(uint32_t)tempi[j]*24u);
  c.clock_follow=0;
  advance(&m,&a,&c,44100u);assert(starts==2);
  assert(clocks==(uint32_t)tempi[j]*24u+(uint32_t)tempi[j]*24u/60u);
  c.seq_run=0;advance(&m,&a,&c,128);assert(stops==2);
 }
 puts("PASS exact 24 PPQN MIDI clock over 60s at 30/89/121/137/240 BPM; master/slave stop and resume");
}
