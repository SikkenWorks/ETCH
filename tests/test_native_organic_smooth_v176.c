/* SPDX-License-Identifier: GPL-3.0-only
 * V1.76 fixed-point drift timing/continuity regression, no hardware implied.
 */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "../firmware/fm1-native/native_organic.h"
static int abs_i32(int v) { return v<0?-v:v; }
int main(void){
 assert(etch_organic_interval(0u)==396900u);
 assert(etch_organic_interval(100u)==110300u);
 assert(etch_organic_duration_ticks(0u)>etch_organic_duration_ticks(100u));
 assert(etch_organic_pitch_inc(123456789u,0)==123456789u);
 uint32_t seed=0xABCDEF12u;
 int checked=0;
 for(int rate=0;rate<=100;rate++){
  uint16_t ticks=etch_organic_duration_ticks((uint8_t)rate);
  assert(ticks>=3400u && ticks<17000u);
  for(int scene=0;scene<24;scene++){
   seed=etch_organic_hash(seed);
   int32_t start=etch_organic_target(seed,100u);
   seed=etch_organic_hash(seed);
   int32_t goal=etch_organic_target(seed,100u);
   int32_t previous=start;
   assert(etch_organic_smooth_pitch(start,goal,0,ticks)==start);
   assert(etch_organic_smooth_pitch(start,goal,ticks,ticks)==goal);
   for(uint32_t i=1;i<=ticks;i++){
    int32_t v=etch_organic_smooth_pitch(start,goal,(uint16_t)i,ticks);
    int diff=v-previous;
    /* 0.05 cents maximum change in any 32-sample control interval;
       old /16 smoothing jumped by hundreds of fixed-point units. */
    assert(abs_i32(diff)<=2);
    if(goal>=start)assert(diff>=0);
    else assert(diff<=0);
    previous=v;
   }
   /* Segment boundaries align exactly; no jump on new random target. */
   seed=etch_organic_hash(seed);
   int32_t next=etch_organic_target(seed,100u);
   assert(etch_organic_smooth_pitch(goal,next,0,ticks)==previous);
   checked++;
  }
 }
 /* No dead zone at low drift settings, no pitch offset when disabled. */
 for(int d=0;d<=100;d++){
  int32_t value=etch_organic_target(0x1234567u,(uint8_t)d);
  assert(value>=-950&&value<=950);
  if(!d)assert(value==0);
 }
 assert(etch_organic_smooth_pitch(567,0,0,2068u)==567);
 assert(etch_organic_smooth_pitch(567,0,2068u,2068u)==0);
 assert(etch_organic_smooth_pitch(-567,0,2068u,2068u)==0);
 printf("PASS V1.76 smooth organic: %d monotonic segments, zero endpoint jumps, zero-depth exact bypass, 2.5-9s cadence\n",checked);
 return 0;
}
