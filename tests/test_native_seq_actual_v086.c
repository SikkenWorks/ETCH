#include <assert.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_seq_cursor.h"
int main(void){
 for(int length=8;length<=256;length*=2){
  assert(etch_seq_actual_step(1,0,0,255,length,17)==0);
  assert(etch_seq_actual_step(1,1,0,255,length,17)==0);
  for(int i=0;i<length;i++)assert(etch_seq_actual_step(1,1,1,i,length,0)==i);
  assert(etch_seq_actual_step(1,1,1,length-1,length,0)==length-1);
  assert(etch_seq_actual_step(0,1,1,255,length,3)==3);
 }
 puts("PASS V0.86: audio-played step never moves to final step before initial tick, preserves loop wrap");
}
