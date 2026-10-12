/* SPDX-License-Identifier: GPL-3.0-only
 * V0.81 web-sequencer display geometry and held-note extension.
 * No audio/flash operations. Include independently in native host tests. */
#pragma once
#include <stdint.h>
#include "native_performance.h"
static inline uint8_t etch_seq_parity_columns(uint16_t len){return len<=64u?8u:16u;}
static inline uint8_t etch_seq_parity_rows(uint16_t len){
 uint8_t cols=etch_seq_parity_columns(len);
 return (uint8_t)((len+cols-1u)/cols);
}
static inline uint16_t etch_seq_parity_length(uint16_t origin,uint16_t target,uint16_t total){
 if(total<1||total>ETCH_SEQ_STEPS||origin>=total||target<origin)return 1u;
 uint16_t n=(uint16_t)(target-origin+1u);
 if(n>255u)n=255u; /* native note duration is uint8_t */
 return n;
}
static inline uint8_t etch_seq_parity_note_exists(const struct etch_native_pattern *p,
  uint16_t origin,uint8_t note){
 if(!p||origin>=ETCH_SEQ_STEPS)return 0u;
 const struct etch_seq_step *st=&p->steps[origin];
 for(uint8_t i=0;i<st->count && i<ETCH_SEQ_NOTES;i++)if(st->notes[i].note==note)return 1u;
 return 0u;
}
static inline uint8_t etch_seq_parity_extend(struct etch_native_pattern *p,
  uint16_t origin,uint16_t target,uint16_t total,uint8_t pitch){
 if(!p||origin>=total||target<origin||target>=total)return 0u;
 struct etch_seq_step *st=&p->steps[origin];
 for(uint8_t i=0;i<st->count && i<ETCH_SEQ_NOTES;i++)if(st->notes[i].note==pitch){
  st->notes[i].length=(uint8_t)etch_seq_parity_length(origin,target,total);return 1u;
 }
 return 0u;
}
/* Simulator holds: a trigger of length L reaches steps origin+1 ... origin+L-1.
 * Difference array avoids O(steps^2) checks when drawing 256 cells. */
static inline void etch_seq_parity_hold_counts(const struct etch_native_pattern *p,
 uint16_t length,uint8_t holds[ETCH_SEQ_STEPS]){
 uint16_t ends[ETCH_SEQ_STEPS+1u]={0};
 uint16_t begins[ETCH_SEQ_STEPS+1u]={0};
 if(!p||!length||length>ETCH_SEQ_STEPS)return;
 for(uint16_t i=0;i<length;i++){
  const struct etch_seq_step *s=&p->steps[i];
  for(uint8_t n=0;n<s->count&&n<ETCH_SEQ_NOTES;n++){
   uint16_t stop=(uint16_t)(i+s->notes[n].length);
   if(stop>length)stop=length;
   if(stop>i+1u){begins[i+1u]++;ends[stop]++;}
  }
 }
 uint16_t active=0;
 for(uint16_t i=0;i<length;i++){
  active=(uint16_t)(active+begins[i]-ends[i]);
  uint16_t count=(uint16_t)(active+p->steps[i].hold_count);
  holds[i]=(uint8_t)(count>255u?255u:count);
 }
}
