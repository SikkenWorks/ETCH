/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include "../firmware/fm1-native/native_drive_softclip.h"
int main(void){
 for(int32_t x=-65536;x<=65536;x+=127){
  assert(etch_drive_softclip(x,0)==x);
  for(uint32_t d=1;d<=32767;d+=2048){
   int32_t y=etch_drive_softclip(x,d);
   assert(y>=-32768 && y<=32767);
   if(x>0) assert(y>=0);
   if(x<0) assert(y<=0);
  }
 }
 for(uint32_t d=1;d<32768;d+=307){
  int32_t previous=-32769;
  for(int32_t x=-65536;x<=65536;x+=64){
   int32_t y=etch_drive_softclip(x,d);
   assert(y>=previous);
   previous=y;
  }
 }
 assert(etch_drive_softclip(32767,32767)>etch_drive_softclip(16384,32767));
 return 0;
}
