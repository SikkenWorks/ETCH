/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_drive_softclip.h"
int main(void){
 const int32_t extreme[]={INT32_MIN,INT32_MIN+1,-1000000000,-65536,-1,0,1,65536,1000000000,INT32_MAX};
 for(unsigned i=0;i<sizeof(extreme)/sizeof(extreme[0]);++i){
  int32_t x=extreme[i];
  assert(etch_drive_softclip(x,0)==x);
  for(uint32_t d=1;d<=32767;d+=1024){
   int32_t y=etch_drive_softclip(x,d);
   assert(y>=-32767 && y<=32767);
   if(x<0)assert(y<=0);
   if(x>0)assert(y>=0);
  }
 }
 for(uint32_t d=1;d<=32767;d+=1024){
  int32_t last=-32768;
  for(int32_t x=-131072;x<=131072;x+=32){
   int32_t y=etch_drive_softclip(x,d);
   assert(y>=last);last=y;
  }
 }
 for(int x=1;x<=65536;x+=127){
  int32_t a=etch_drive_softclip(x,16384);
  int32_t b=etch_drive_softclip(-x,16384);
  assert(a==-b);
 }
 puts("PASS V1.39 phase 3: drive extreme range, sign, symmetry, monotonicity, bypass");
 return 0;
}
