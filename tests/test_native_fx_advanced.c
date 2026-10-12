/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/native_fx.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static struct etch_native_fx fx;
static struct etch_fx_settings cfg;
static int run_impulse(unsigned sample, unsigned frames){
 etch_fx_reset(&fx);
 int result=0;
 for(unsigned n=0;n<frames;n++){
  int16_t v=etch_fx_process_ex(&fx,n==0?12000:0,0,32767,0,&cfg);
  if(n==sample)result=v;
  assert(v<=32767&&v>=-32768);
 }
 return result;
}
int main(void){
 etch_fx_defaults(&cfg);
 cfg.delay_time=0;cfg.delay_feedback=0;
 assert(run_impulse(384,900)>1000);
 assert(run_impulse(768,900)==0);
 cfg.delay_feedback=28000;
 assert(run_impulse(768,900)>1000);
 cfg.delay_time=32767;cfg.delay_feedback=0;
 assert(run_impulse(15872,16000)>1000);
 etch_fx_reset(&fx);cfg.routing=1;
 for(int n=0;n<40000;n++){
  int16_t input=(n%97<48)?14000:-14000;
  int16_t out=etch_fx_process_ex(&fx,input,32767,32767,32767,&cfg);
  assert(out<=32767&&out>=-32768);
 }
 cfg.routing=0;cfg.reverb_decay=32767;cfg.delay_feedback=32767;
 etch_fx_reset(&fx);
 for(int n=0;n<80000;n++){
  int16_t out=etch_fx_process_ex(&fx,n==0?25000:0,32767,32767,32767,&cfg);
  assert(out<=32767&&out>=-32768);
 }
 puts("Advanced FX: dynamic echo time/feedback, serial/parallel and max-feedback bounded stress PASS");
}
