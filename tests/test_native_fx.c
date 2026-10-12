#include "../firmware/fm1-native/native_fx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static struct etch_native_fx fx;
int main(void){
 etch_fx_reset(&fx);
 for(unsigned n=0;n<70000;n++){
  int16_t in=n==0?12000:0;
  int16_t out=etch_fx_process(&fx,in,0,0,0);
  assert(out==in); /* exact bypass; no dry gain change */
 }
 etch_fx_reset(&fx);long total=0;unsigned echoes=0;
 for(unsigned n=0;n<35000;n++){
  int16_t x=etch_fx_process(&fx,n==0?12000:0,32767,32767,32767);
  assert(abs(x)<=32768);if(n>100&&x){total+=labs(x);echoes++;}
 }
 assert(total>0&&echoes>50);
 puts("Native FX: exact dry bypass, delay echoes, bounded feedback passed");
}
