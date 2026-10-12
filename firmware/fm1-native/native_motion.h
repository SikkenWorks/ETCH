/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH motion recorder: four 64-position Q15 lanes, 100 ms per position.
 * No heap, blocking operations or display work from realtime audio.
 */
#pragma once
#include <stdint.h>
#define ETCH_MOTION_LANES 4u
#define ETCH_MOTION_STEPS 64u
#define ETCH_MOTION_SAMPLES_PER_STEP 4410u
struct etch_motion_clip {
 uint8_t length,mask;
 uint16_t lane[ETCH_MOTION_LANES][ETCH_MOTION_STEPS];
};
static inline uint8_t etch_motion_valid(const struct etch_motion_clip *c){
 if(!c||c->length>ETCH_MOTION_STEPS||c->mask>15u)return 0;
 for(uint32_t k=0;k<ETCH_MOTION_LANES;k++)for(uint32_t i=0;i<c->length;i++)
  if(c->lane[k][i]>32767u)return 0;
 return 1;
}
static inline void etch_motion_clear(struct etch_motion_clip *c){
 if(!c)return;
 c->length=0;c->mask=0;
 for(uint32_t k=0;k<ETCH_MOTION_LANES;k++)for(uint32_t i=0;i<ETCH_MOTION_STEPS;i++)c->lane[k][i]=0;
}
/* Each tick records all four sources, but only touched lanes play back. */
static inline void etch_motion_capture(struct etch_motion_clip *c,const uint16_t src[4],uint8_t touched){
 if(!c||!src||c->length>=ETCH_MOTION_STEPS)return;
 for(uint32_t k=0;k<4u;k++)c->lane[k][c->length]=src[k]>32767u?32767u:src[k];
 c->mask|=touched&15u;
 ++c->length;
}
/* Interpolate around the wrap instead of jumping from last point to first.
 * This is used by the audio render thread, never by the flash codec. */
static inline uint16_t etch_motion_at(const struct etch_motion_clip *c,uint8_t lane,uint32_t sample){
 if(!c||!c->length||lane>=4u)return 0;
 uint32_t total=(uint32_t)c->length*ETCH_MOTION_SAMPLES_PER_STEP;
 uint32_t pos=sample%total;
 uint32_t idx=pos/ETCH_MOTION_SAMPLES_PER_STEP;
 uint32_t fraction=pos%ETCH_MOTION_SAMPLES_PER_STEP;
 int32_t a=c->lane[lane][idx];
 int32_t b=c->lane[lane][(idx+1u)%c->length];
 return (uint16_t)(a+((b-a)*(int32_t)fraction)/(int32_t)ETCH_MOTION_SAMPLES_PER_STEP);
}
