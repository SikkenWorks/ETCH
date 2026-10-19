/* SPDX-License-Identifier: GPL-3.0-only
 * Compare checked and per-DMA-block-prevalidated matrix math on every route
 * combination, with randomized envelopes and signed-depth values.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_mod_matrix.h"
static uint32_t seed=0x5ea7faceu;
static uint32_t rand32(void){seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;return seed;}
int main(void){
 for(uint32_t i=0;i<400000u;i++){
  struct etch_mod_matrix m={0};
  for(int j=0;j<4;j++){
   m.route[j].source=(uint8_t)(rand32()%7u);
   m.route[j].target=(uint8_t)(rand32()%4u);
   m.route[j].amount=(uint8_t)(rand32()%201u);
  }
  uint8_t enabled=etch_matrix_active(&m);
  uint8_t t=(uint8_t)(rand32()%4u),v=(uint8_t)(rand32()%128u),pr=(uint8_t)(rand32()%128u),note=(uint8_t)(rand32()%128u);
  uint16_t base=(uint16_t)(rand32()%32768u),env=(uint16_t)(rand32()%32768u),living=(uint16_t)(rand32()%32768u);
  int16_t c=(int16_t)(rand32()&65535u);
  uint16_t classic=etch_matrix_apply_ex(&m,t,base,v,env,c,living,pr,note);
  uint16_t cached=enabled?etch_matrix_apply_active_ex(&m,t,base,v,env,c,living,pr,note):base;
  assert(classic==cached);
 }
 puts("PASS 400000 randomized Mod Matrix checked/cached output comparisons");
 return 0;
}
