/* SPDX-License-Identifier: GPL-3.0-only
 * Long-running simulated DAW MIDI validation. Not a physical DAW/device test.
 */
#include "../firmware/fm1-native/native_performance_midi.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint64_t frame, clocks, starts, stops, ons, offs, max_error;
static uint8_t queue_full;
static uint32_t tempo;
static int send(uint32_t packet,void *unused){
 (void)unused;
 if(queue_full)return 0;
 uint8_t status=(uint8_t)(packet>>8);
 if(status==0xf8){
  ++clocks;
  uint64_t ideal=(clocks*44100ull*60ull)/(tempo*24ull);
  uint64_t d=frame>ideal?frame-ideal:ideal-frame;
  if(d>max_error)max_error=d;
 }else if(status==0xfa)starts++;
 else if(status==0xfc)stops++;
 else if((status&0xf0)==0x90)ons++;
 else if((status&0xf0)==0x80)offs++;
 return 1;
}
static void tick(struct etch_perf_midi *m,struct etch_native_audio *a,
                 struct etch_native_controls *c,uint32_t frames){
 frame+=frames;
 etch_perf_midi_update(m,a,c,1,frames,send,0);
}
int main(void){
 struct etch_perf_midi m;
 struct etch_native_audio a;
 struct etch_native_controls c;
 memset(&a,0,sizeof a);memset(&c,0,sizeof c);
 etch_perf_midi_reset(&m);
 tempo=137;c.tempo=(uint8_t)tempo;c.seq_run=1;
 /* 60 minutes of 44.1kHz blocks, no outgoing-clock loss, error <= one block. */
 uint64_t frames=44100ull*3600ull;
 while(frames){
  uint32_t chunk=frames>=128?128:(uint32_t)frames;
  tick(&m,&a,&c,chunk);frames-=chunk;
 }
 assert(starts==1&&stops==0);
 assert(clocks==137ull*24ull*60ull);
 assert(max_error<=128u);
 c.seq_run=0;tick(&m,&a,&c,128);assert(stops==1);
 /* Stress 8 simultaneously playing voices, frequent replacements and short
  * synthetic USB stalls. Releases must be kept pending and sent first. */
 etch_perf_midi_reset(&m);
 memset(&a,0,sizeof a);c.seq_run=0;
 ons=offs=0;queue_full=0;
 for(uint32_t j=0;j<10000;j++){
  uint8_t k=(uint8_t)(j%ETCH_NATIVE_VOICES);
  a.voice[k].active=1;a.voice[k].held=1;a.voice[k].down=1;
  a.voice[k].velocity=110;
  a.voice[k].note=4096u+(uint32_t)((j/ETCH_NATIVE_VOICES+k)%48u+36u);
  queue_full=(uint8_t)((j%131u)==0u);
  tick(&m,&a,&c,128);
  queue_full=0;tick(&m,&a,&c,128);
 }
 for(uint32_t k=0;k<ETCH_NATIVE_VOICES;k++){
  a.voice[k].active=0;a.voice[k].held=0;a.voice[k].down=0;
 }
 tick(&m,&a,&c,128);
 for(uint8_t k=0;k<ETCH_NATIVE_VOICES;k++)assert(!m.sent[k]);
 assert(ons==offs);
 /* Switching to slave must not echo clock or restart automatically. */
 c.clock_follow=1;c.seq_run=1;
 uint64_t old=clocks;uint64_t oldstarts=starts;
 for(uint32_t j=0;j<1000;j++)tick(&m,&a,&c,128);
 assert(clocks==old&&starts==oldstarts);
 printf("PASS MIDI 60-minute 137 BPM clock %llu ticks; max callback quantization %llu frames; 10000 voice replacements %llu on/off, queue backpressure, slave no echo\n",
   (unsigned long long)clocks,(unsigned long long)max_error,(unsigned long long)ons);
}
