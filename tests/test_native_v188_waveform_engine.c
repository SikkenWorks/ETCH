/* SPDX-License-Identifier: GPL-3.0-only
 * V1.88 non-realtime wave shaping, audio table identity, undo and compatibility.
 */
#include "../firmware/fm1-native/wave_editor.h"
#include "../firmware/fm1-native/native_wave_shape.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct etch_wave_editor e;
static struct etch_native_bank bank_a,bank_b;
static int32_t wavetable_diff(uint8_t w,uint8_t mip){
 int32_t sum=0;for(uint32_t i=0u;i<512u;i++){
  int32_t d=bank_a.table[w][mip][i]-(int32_t)bank_b.table[w][mip][i];sum+=d<0?-d:d;
 }
 return sum;
}
static void test_zero_is_noop(uint8_t tool){
 etch_editor_init(&e);int16_t original[3][128];memcpy(original,e.points,sizeof original);
 uint32_t rev=e.revision;
 etch_editor_tool(&e,tool,0);
 assert(rev==e.revision&&e.undo_count==0u);
 assert(!memcmp(original,e.points,sizeof original));
}
static void test_each_wave(uint8_t tool,int16_t amt){
 etch_editor_init(&e);
 for(uint8_t w=0;w<3u;w++){
  /* Force non-trivial asymmetric waveform: two nonidentical halves. */
  for(uint32_t i=0u;i<128u;i++)e.points[w][i]=(int16_t)(((int32_t)((i*37u+w*13u)%128u)-64)*430);
 }
 etch_editor_compile(&e,&bank_a);
 int16_t original[3][128];memcpy(original,e.points,sizeof original);
 e.wave=1;
 uint32_t before=e.revision;
 etch_editor_tool(&e,tool,amt);
 assert(e.revision==before+1u&&e.dirty&&e.undo_count==1u);
 assert(memcmp(original[1],e.points[1],sizeof(original[1]))!=0);
 assert(memcmp(original[0],e.points[0],sizeof(original[0]))==0);
 assert(memcmp(original[2],e.points[2],sizeof(original[2]))==0);
 etch_editor_compile(&e,&bank_b);
 assert(wavetable_diff(1u,0u)>12000);
 assert(wavetable_diff(0u,0u)==0 && wavetable_diff(2u,0u)==0);
 for(uint8_t mip=0u;mip<6u;mip++)for(uint32_t i=0u;i<512u;i++)
  assert(abs(bank_b.table[1][mip][i])<=29000);
 etch_editor_undo(&e);
 assert(!memcmp(original,e.points,sizeof original));
 etch_editor_redo(&e);
 assert(memcmp(original[1],e.points[1],sizeof(original[1]))!=0);
}
static void test_symmetry_extremes(void){
 int16_t input[128],output[128];
 for(int i=0;i<128;i++)input[i]=(int16_t)((i%64)*331-11000+(i>=64?1600:0));
 etch_wave_shape_preview(input,output,ETCH_WAVE_TOOL_HALF_SYMM,127);
 for(uint32_t i=0;i<64u;i++){
  assert(output[i]==input[i]);
  assert(output[i+64u]==-input[i]);
 }
 etch_wave_shape_preview(input,output,ETCH_WAVE_TOOL_HALF_SYMM,-127);
 for(uint32_t i=0;i<64u;i++)assert(output[i+64u]==input[i]);
}
static void test_monotonic_phase_warp(void){
 int16_t saw[128],out[128];for(int i=0;i<128;i++)saw[i]=(int16_t)(i*400-25000);
 for(int amount=-127;amount<=127;amount+=13){
  etch_wave_shape_preview(saw,out,ETCH_WAVE_TOOL_PHASE_WARP,(int16_t)amount);
  assert(out[0]==saw[0]);
  for(int i=1;i<128;i++)assert(out[i]>=out[i-1]);
 }
}
int main(void){
 test_zero_is_noop(ETCH_WAVE_TOOL_PHASE_WARP);
 test_zero_is_noop(ETCH_WAVE_TOOL_HALF_SYMM);
 test_each_wave(ETCH_WAVE_TOOL_PHASE_WARP,101);
 test_each_wave(ETCH_WAVE_TOOL_PHASE_WARP,-101);
 test_each_wave(ETCH_WAVE_TOOL_HALF_SYMM,127);
 test_each_wave(ETCH_WAVE_TOOL_HALF_SYMM,-127);
 test_symmetry_extremes();test_monotonic_phase_warp();
 puts("PASS V1.88: 128-point phase warp/half symmetry, bounded PCM mip tables, no-op neutral, wave isolation, undo/redo, monotone resampling");
 return 0;
}
