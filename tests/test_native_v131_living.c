/* SPDX-License-Identifier: GPL-3.0-only: LIVING fundamental vs standard mip. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_living.h"
#include "../firmware/fm1-native/native_tables.h"
static int32_t table_read(uint32_t mip,uint32_t p){
 const int16_t *t=etch_tables[1][mip];uint32_t i=p>>23;
 int32_t f=(int32_t)((p>>8)&32767u);
 return t[i]+(t[(i+1u)&511u]-t[i])*f/32768;
}
int main(void){
 struct etch_living_settings cfg={100,100,0,0};
 assert(etch_living_valid(&cfg));
 assert(etch_living_period(100u)<etch_living_period(0u));
 /* The prior inc*4 method could return the same bank for lower notes.
  * New reference must use true fundamental-only mip regardless of note. */
 int differences=0;
 for(uint32_t i=0;i<512u;i++)if(table_read(0,i<<23)!=table_read(5,i<<23))differences++;
 assert(differences>256);
 uint16_t weight=etch_living_weight(etch_living_period(100),5,127,&cfg);
 assert(weight==32767);
 int mod_diff=0;
 for(uint32_t i=0;i<512u;i++){
  int32_t a=table_read(0,i<<23),b=table_read(5,i<<23);
  if(etch_living_mix((int16_t)a,(int16_t)b,weight,0)!=(int16_t)a)mod_diff++;
  assert(etch_living_mix((int16_t)a,(int16_t)b,0,0)==(int16_t)a);
 }
 assert(mod_diff>256);
 puts("PASS LIVING V1.31 audible at low notes with harmonic-rich waveform and exact bypass");
}
