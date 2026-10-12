/* ETCH V1.02 DSP all-effects simultaneous/edge parameter stress.
 * Host only: UBSAN/ASAN detects invalid arithmetic and buffer access.
 */
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_fx.h"
static struct etch_native_fx fx;
int main(void){
 struct etch_fx_settings c; etch_fx_defaults(&c);
 c.comp_ratio=32767; c.comp_threshold=0; c.comp_attack=0; c.comp_release=0;
 c.trem_depth=c.trem_mix=c.trem_rate=c.trem_shape=32767;
 c.phaser_rate=c.phaser_depth=c.phaser_feedback=c.phaser_mix=32767;
 c.flanger_rate=c.flanger_depth=c.flanger_feedback=c.flanger_mix=32767;
 c.decim_bits=0;c.decim_rate=c.decim_drive=c.decim_mix=32767;
 c.folder_fold=c.folder_symmetry=c.folder_mix=32767;c.folder_tone=0;
 etch_fx_reset(&fx);
 uint32_t acc=0;
 for(int i=0;i<250000;i++){
  int16_t v=(int16_t)((uint32_t)i*1664525u>>16);
  if(i%17333==0){ c.folder_symmetry=(i&1)?0:32767; c.decim_bits=(i&1)?0:32767; }
  int16_t left,right;etch_fx_process_stereo_ex(&fx,v,22000,21000,20000,&c,&left,&right);
  acc+=(uint16_t)left;acc^=(uint16_t)right;
 }
 printf("PASS V1.02 simultaneous FX stress, 250000 samples, checksum %u\n",acc);
 return 0;
}
