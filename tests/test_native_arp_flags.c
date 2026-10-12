#include "../firmware/fm1-native/native_performance.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 uint8_t r=99,o=99,g=99;
 etch_arp_unpack_flags(0,&r,&o,&g);
 assert(r==2&&o==1&&g==2); /* older V6 patch */
 for(uint8_t rate=0;rate<4;rate++)for(uint8_t oct=1;oct<=4;oct++)for(uint8_t gate=0;gate<4;gate++){
  uint8_t flags=etch_arp_pack_flags(rate,oct,gate);
  assert(flags&0x80u);
  etch_arp_unpack_flags(flags,&r,&o,&g);
  assert(r==rate&&o==oct&&g==gate);
 }
 for(uint8_t rate=0;rate<4;rate++)for(uint8_t oct=5;oct<=6;oct++)for(uint8_t gate=0;gate<4;gate++){
  uint8_t flags=etch_arp_pack_flags(rate,oct,gate);
  assert((flags&0xc0u)==0xc0u);
  etch_arp_unpack_flags(flags,&r,&o,&g);
  assert(r==rate&&o==oct&&g==gate);
 }
 assert(etch_arp_octave_turn(1u,-1)==5u);
 assert(etch_arp_octave_turn(5u,-1)==6u);
 assert(etch_arp_octave_turn(6u,1)==5u);
 assert(etch_arp_octave_turn(5u,1)==1u);
 assert(etch_arp_octave_display(6u)==-2);
 assert(etch_arp_octave_display(5u)==-1);
 assert(etch_arp_pack_flags(200,0,100)==etch_arp_pack_flags(2,1,2));
 puts("PASS ARP rate/octaves/gate 96 combinations including negative octave ranges and legacy V6 decoding");
 return 0;
}
