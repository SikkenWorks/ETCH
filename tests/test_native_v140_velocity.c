/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_expressive.h"
int main(void){
 for(int v=0;v<=127;v++){
  assert(etch_expressive_velocity(v,0)==v);
  assert(etch_expressive_velocity(v,3)==v);
  assert(etch_expressive_velocity(v,1)>=v);
  assert(etch_expressive_velocity(v,2)<=v);
  if(v){
   for(int c=0;c<3;c++)assert(etch_expressive_velocity(v,c)>=etch_expressive_velocity(v-1,c));
  }
 }
 for(int c=0;c<3;c++){
  assert(etch_expressive_velocity(0,c)==0);
  assert(etch_expressive_velocity(127,c)==127);
 }
 puts("PASS V1.40 velocity curves: legacy exact, soft/hard, bounds, monotonicity");
 return 0;
}
