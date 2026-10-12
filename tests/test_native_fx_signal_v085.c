/* V0.85: control knob changes reach the *audible* processed samples. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../firmware/fm1-native/native_fx.h"
static struct etch_native_fx fx;
static int16_t run(uint32_t frame,int mode,struct etch_fx_settings *cfg){
 int16_t in=frame<20000u?(int16_t)(((int32_t)(frame%79u)-39)*180):0;
 uint16_t ch=mode==1?26000:0,del=mode==2?26000:0,rev=mode==3?26000:0;
 return etch_fx_process_ex(&fx,in,ch,del,rev,cfg);
}
static void measure(int mode,uint64_t *difference,uint64_t *tail){
 struct etch_fx_settings cfg;etch_fx_defaults(&cfg);etch_fx_reset(&fx);
 *difference=*tail=0;
 for(uint32_t i=0;i<55000u;i++){
  int16_t original=i<20000u?(int16_t)(((int32_t)(i%79u)-39)*180):0;
  int16_t processed=run(i,mode,&cfg);
  if(i>=2000u&&i<20000u){int d=processed-original;*difference+=(uint64_t)abs(d);}
  if(i>=20000u)*tail+=(uint64_t)abs(processed);
 }
}
int main(void){
 uint64_t diff=0,tail=0;
 measure(0,&diff,&tail);assert(diff==0&&tail==0);
 for(int mode=1;mode<=3;mode++){
  measure(mode,&diff,&tail);
  printf("effect %d: wet sample difference=%llu, release tail=%llu\n",mode,(unsigned long long)diff,(unsigned long long)tail);
  assert(diff>100000);assert(tail>10000);
 }
 puts("PASS: CHORUS, DELAY, REVERB change PCM and retain audible note-release tails");
}
