/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.25 Living Waves: independent per-note harmonic motion.
 * Band-limited table-to-table spectral shaping, no allocations/float in ISR.
 */
#pragma once
#include <stdint.h>
struct etch_living_settings {uint8_t depth,rate,color,vary;};
static inline uint8_t etch_living_valid(const struct etch_living_settings *p){
 return p&&p->depth<=100u&&p->rate<=100u&&p->color<=100u&&p->vary<=100u;
}
/* Control ticks occur once per 32 samples. A full cycle is ~1.5-9.4 seconds. */
static inline uint16_t etch_living_period(uint8_t rate){
 return (uint16_t)(6500u-(uint32_t)(rate<=100u?rate:100u)*55u);
}
static inline uint16_t etch_living_weight(uint16_t age,uint32_t birth,
 uint8_t velocity,const struct etch_living_settings *p){
 if(!etch_living_valid(p)||!p->depth)return 0u;
 uint32_t period=etch_living_period(p->rate);
 uint32_t offset=p->vary?((birth*2654435761u>>16)%period)*p->vary/100u:0u;
 uint32_t phase=((uint32_t)age+offset)%(period*2u);
 uint32_t triangle=phase<period?phase:period*2u-phase;
 /* Vary adds phase individuality; velocity adds natural touch sensitivity.
  * Unity velocity scaling at 127: no unexpected attenuation of held notes. */
 uint32_t vel=(uint32_t)velocity*40u/127u+60u;
 uint32_t amount=(triangle*32767u)/period;
 return (uint16_t)(amount*(uint32_t)p->depth/100u*vel/100u);
}
static inline int16_t etch_living_mix(int16_t full,int16_t reduced,uint16_t weight,uint8_t color){
 if(!weight)return full;
 /* COLOR 0: subtract upper harmonics.  COLOR 100: emphasize the same
  * band-limited upper partials. Dry sample is unchanged on bypass. */
 int32_t detail=(int32_t)full-(int32_t)reduced;
 int32_t signed_tilt=(int32_t)color*200u/100u-100; /* -100..+100 */
 int32_t band=(detail*(int32_t)weight)/32767;
 int32_t delta=(band*signed_tilt)/100;
 int32_t v=(int32_t)full+delta;
 return (int16_t)(v>32767?32767:v<-32768?-32768:v);
}
