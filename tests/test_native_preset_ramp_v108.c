#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_preset_ramp.h"
int main(void){
 struct etch_preset_ramp r={0};
 etch_preset_ramp_reset(&r);
 int32_t a=12000,b=-8000;
 etch_preset_ramp_frame(&r,&a,&b);
 etch_preset_ramp_begin(&r);
 for(unsigned i=0;i<ETCH_PRESET_RAMP_SAMPLES;i++){
  int32_t x=-14000,y=14000;
  etch_preset_ramp_frame(&r,&x,&y);
  if(i==0){assert(x==12000&&y==-8000);}
  assert(x>=-14000 && x<=12000 && y>=-8000 && y<=14000);
 }
 assert(r.remaining==0);
 a=-14000;b=14000;etch_preset_ramp_frame(&r,&a,&b);
 assert(a==-14000&&b==14000);
 /* A retrigger starts from the exact last transmitted sample. */
 etch_preset_ramp_begin(&r);a=10000;b=-10000;
 etch_preset_ramp_frame(&r,&a,&b);assert(a==-14000&&b==14000);
 printf("PASS preset scroll fade and continuity\\n");
 return 0;
}
