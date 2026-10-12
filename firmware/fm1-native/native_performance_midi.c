/* SPDX-License-Identifier: GPL-3.0-only */
#include "native_performance_midi.h"
static uint32_t event(uint8_t status,uint8_t n,uint8_t vel){return (uint32_t)(status==0x90?9:8)|((uint32_t)status<<8)|((uint32_t)n<<16)|((uint32_t)vel<<24);}
static uint32_t realtime(uint8_t status){return 0x0fu|((uint32_t)status<<8);}
void etch_perf_midi_reset(struct etch_perf_midi *m){
 if(!m)return;
 for(uint8_t i=0;i<ETCH_NATIVE_VOICES;i++){m->sent[i]=0;m->note[i]=0;m->birth[i]=0;}
 m->transport=0;m->clock_phase=0;
}
void etch_perf_midi_flush(struct etch_perf_midi *m,etch_perf_emit emit,void *context){
 if(!m||!emit)return;
 for(uint8_t i=0;i<ETCH_NATIVE_VOICES;i++)if(m->sent[i]){
  if(!emit(event(0x80,m->note[i],0),context))return;
  m->sent[i]=0;
 }
 if(m->transport){if(emit(realtime(0xfc),context))m->transport=0;}
}
void etch_perf_midi_update(struct etch_perf_midi *m,const struct etch_native_audio *audio,
 const struct etch_native_controls *c,uint8_t connected,uint32_t frames,
 etch_perf_emit emit,void *context){
 if(!m||!audio||!c)return;
 if(!connected){etch_perf_midi_reset(m);return;}
 if(!emit)return;
 /* If local transport is enabled, suppress outbound clock when slaved to
    incoming clock, avoiding feedback loops. */
 uint8_t running=(uint8_t)(c->seq_run&&!c->clock_follow);
 /* A stopped transport must release voices and send MIDI Stop even when USB
    backpressure prevented the first attempt. */
 if(!running&&m->transport){
  etch_perf_midi_flush(m,emit,context);
  if(m->transport)return;
 }
 /* Note-offs first. Hold a pending off across queue-full events; do not
    overwrite it with another note-on from a reused voice slot. */
 for(uint8_t i=0;i<ETCH_NATIVE_VOICES;i++)if(m->sent[i]){
  const struct etch_native_voice *v=&audio->voice[i];
  uint8_t playing=(uint8_t)(v->active&&v->note>=4096u&&v->note<4224u&&v->held&&v->down);
  uint8_t pitch=(uint8_t)(v->note&127u);
  if(!playing||pitch!=m->note[i]||v->birth!=m->birth[i]){
   if(!emit(event(0x80,m->note[i],0),context))return;
   m->sent[i]=0;
  }
 }
 if(running&&!m->transport){if(emit(realtime(0xfa),context))m->transport=1;}
 for(uint8_t i=0;i<ETCH_NATIVE_VOICES;i++){
  const struct etch_native_voice *v=&audio->voice[i];
  if(!m->sent[i]&&v->active&&v->note>=4096u&&v->note<4224u&&v->held&&v->down){
   uint8_t note=(uint8_t)(v->note&127u);
   if(!emit(event(0x90,note,v->velocity?v->velocity:1),context))return;
   m->note[i]=note;m->birth[i]=v->birth;m->sent[i]=1;
  }
 }
 if(running&&m->transport){
  /* Rational 24 PPQN phase: never round the samples-per-tick interval.
   * At 44.1 kHz the denominator is exactly one minute of samples.
   * 128-frame blocks at <=240 BPM cannot overflow this uint32_t phase. */
  uint32_t tempo=c->tempo<30?120:c->tempo;
  const uint32_t tick_denominator=44100u*60u;
  m->clock_phase+=frames*tempo*24u;
  while(m->clock_phase>=tick_denominator){
   /* Drop late realtime clocks under backpressure; preserve note-off priority. */
   if(!emit(realtime(0xf8),context)){
    m->clock_phase%=tick_denominator;
    break;
   }
   m->clock_phase-=tick_denominator;
  }
 }else m->clock_phase=0;
}
