#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../firmware/fm1-native/native_fx.h"
static struct etch_native_fx dry,chorus,delay,reverb;
int main(void){
 struct etch_fx_settings cfg;etch_fx_defaults(&cfg);
 etch_fx_reset(&dry);etch_fx_reset(&chorus);etch_fx_reset(&delay);etch_fx_reset(&reverb);
 /* Evolving multi-harmonic audio rather than repeating DC/silence; wet FX are
  * compared across a full reverb window, not just the first transient. */
 uint32_t phase=1;unsigned long long diff[3]={0};
 for(unsigned n=0;n<44100;n++){
  phase=phase*1664525u+1013904223u;
  int16_t sample=(int16_t)(((int32_t)(phase>>16))/3);
  int16_t a=etch_fx_process_ex(&dry,sample,0,0,0,&cfg);
  int16_t b=etch_fx_process_ex(&chorus,sample,28000,0,0,&cfg);
  int16_t c=etch_fx_process_ex(&delay,sample,0,28000,0,&cfg);
  int16_t d=etch_fx_process_ex(&reverb,sample,0,0,28000,&cfg);
  if(n>9000){diff[0]+=(unsigned)abs((int)b-a);diff[1]+=(unsigned)abs((int)c-a);diff[2]+=(unsigned)abs((int)d-a);}
 }
 printf("FX audio differences chorus=%llu delay=%llu reverb=%llu\n",diff[0],diff[1],diff[2]);
 if(diff[0]<500000u||diff[1]<500000u||diff[2]<500000u){fprintf(stderr,"one or more wet FX have no audible DSP effect\n");return 1;}
 return 0;
}
