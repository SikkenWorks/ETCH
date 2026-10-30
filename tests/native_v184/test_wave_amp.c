/* ETCH V1.84: 100% bipolar->unipolar gain and packed PJT32 fields. */
#include <assert.h>
#include <stdio.h>
#include "native_wave_amp.h"
int main(void){
 assert(etch_wamp_unipolar(-32768)==0u);
 assert(etch_wamp_unipolar(32767)==32768u);
 assert(etch_wamp_unipolar(0)==16384u);
 assert(etch_wamp_source(1,111,-222,12345)==111);
 assert(etch_wamp_source(2,111,-222,12345)==-222);
 assert(etch_wamp_source(3,-32768,32767,0)==-32768);
 assert(etch_wamp_source(3,-32768,32767,32767)>=32764);
 for(unsigned mode=0;mode<4;mode++)for(unsigned depth=0;depth<64;depth++){
  uint8_t wave=etch_wamp_pack_wave(2u,(uint8_t)mode);
  uint8_t velo=etch_wamp_pack_velocity(1u,(uint8_t)depth);
  assert((wave&3u)==2u && etch_wamp_mode(wave)==mode);
  assert(etch_wamp_velocity(velo)==1u && etch_wamp_depth6(velo)==depth);
  uint16_t d=etch_wamp_q15((uint8_t)depth);
  for(int signal=-32768;signal<=32767;signal+=101){
   for(int sample=-32767;sample<=32767;sample+=4003){
    int32_t x=etch_wamp_apply(sample,signal,d);
    assert((sample>=0&&x>=0)||(sample<0&&x<=0));
    assert((x>=0?x:-x)<=(sample>=0?sample:-sample));
    assert(etch_wamp_apply(sample,signal,0)==sample);
   }
  }
 }
 assert(etch_wamp_apply(30000,-32768,32768u)==0);
 assert(etch_wamp_apply(30000,32767,32768u)==30000);
 assert(etch_wamp_apply(-30000,32767,32768u)==-30000);
 assert(etch_wamp_apply(-30000,-32768,32768u)==0);
 puts("PASS WAVE AMP V1.84: 64 packed settings, phase waveform mapping, zero/off parity and bounded gain");
 return 0;
}
