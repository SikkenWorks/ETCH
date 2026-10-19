/* SPDX-License-Identifier: GPL-3.0-only
 * V1.44: bounded project-scoped performance macros and two scenes.
 * Host-testable; no heap, no ISR writes or extra DSP buffers.
 */
#pragma once
#include <stdint.h>
#define ETCH_PERF_MACROS 4u
#define ETCH_PERF_SCENES 2u
/* 0 MORPH, 1 ETCH, 2 CUTOFF, 3 DRIVE, 4 SPACE. */
#define ETCH_PERF_TARGETS 5u
struct etch_perf_scenes {
 uint8_t target[ETCH_PERF_MACROS];
 uint8_t value[ETCH_PERF_SCENES][ETCH_PERF_MACROS];
 uint8_t valid; /* bit 0: A, bit 1: B */
};
_Static_assert(sizeof(struct etch_perf_scenes)==13u,"performance storage ABI");
static inline void etch_perf_init(struct etch_perf_scenes *s){
 if(!s)return;
 for(uint8_t i=0;i<ETCH_PERF_MACROS;i++){
  s->target[i]=i;
  for(uint8_t j=0;j<ETCH_PERF_SCENES;j++)s->value[j][i]=0;
 }
 s->valid=0;
}
static inline uint8_t etch_perf_valid(const struct etch_perf_scenes *s){
 if(!s||(s->valid&~3u))return 0;
 for(uint8_t i=0;i<ETCH_PERF_MACROS;i++){
  if(s->target[i]>=ETCH_PERF_TARGETS)return 0;
  for(uint8_t j=0;j<ETCH_PERF_SCENES;j++)if(s->value[j][i]>100u)return 0;
 }
 return 1;
}
static inline uint16_t etch_perf_q15(uint8_t pct){return (uint16_t)(((uint32_t)pct*32767u+50u)/100u);}
static inline uint8_t etch_perf_percent(uint16_t value){return (uint8_t)(((uint32_t)(value>32767u?32767u:value)*100u+16383u)/32767u);}
/* CUTOFF is normalized over the hardware's 20..18000 Hz musical range.
 * The native filter's parameter slew remains responsible for click-free changes. */
static inline uint16_t etch_perf_cutoff(uint8_t pct){return (uint16_t)(20u+((uint32_t)pct*17980u+50u)/100u);}
static inline uint8_t etch_perf_cutoff_pct(uint16_t hz){
 uint32_t value=hz<20u?20u:hz>18000u?18000u:hz;
 return (uint8_t)(((value-20u)*100u+8990u)/17980u);
}
static inline uint8_t etch_perf_change(uint8_t value,int32_t turns){
 int32_t next=(int32_t)value+turns;
 return (uint8_t)(next<0?0:next>100?100:next);
}
static inline void etch_perf_capture(struct etch_perf_scenes *s,uint8_t scene,const uint8_t values[4]){
 if(!s||scene>=2)return;
 for(uint8_t k=0;k<4;k++)s->value[scene][k]=values[k]>100u?100u:values[k];
 s->valid|=(uint8_t)(1u<<scene);
}
static inline uint8_t etch_perf_recall(const struct etch_perf_scenes *s,uint8_t scene,uint8_t values[4]){
 if(!etch_perf_valid(s)||scene>=2||!(s->valid&(1u<<scene)))return 0;
 for(uint8_t k=0;k<4;k++)values[k]=s->value[scene][k];
 return 1;
}
