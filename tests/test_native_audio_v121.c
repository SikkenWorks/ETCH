/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_mip_smoothing.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct etch_native_audio s;
static struct etch_native_controls c;
static int16_t pcm[256];
static void init(void){
 etch_native_reset(&s);memset(&c,0,sizeof(c));etch_fx_defaults(&c.fx_settings);
 c.gain=32767;c.tempo=120;c.env_enabled=1;
 c.attack_step=c.decay_step=0x7fff0000u;c.sustain_level=32767;
 c.release_step=0x01000000u;
}
int main(void){
 const uint32_t limits[]={30198988u,60397977u,120795955u,241591910u,966367641u};
 for(int j=0;j<5;j++){
  uint32_t boundary=limits[j],start=boundary-(boundary>>3);
  assert(etch_mip_boundary_alpha(start,boundary)==0u);
  assert(etch_mip_boundary_alpha(start+(boundary-start)/32768u+1u,boundary)>0u);
  assert(etch_mip_boundary_alpha(boundary,boundary)==32768u);
  assert(etch_mip_boundary_alpha(boundary+1u,boundary)==32768u);
  uint32_t prev=0u;
  for(uint32_t i=0;i<=1000;i++){
   uint32_t inc=start+(uint32_t)(((uint64_t)(boundary-start)*i)/1000);
   uint32_t alpha=etch_mip_boundary_alpha(inc,boundary);
   assert(alpha>=prev&&alpha<=32768u);prev=alpha;
  }
 }
 init();
 c.filter_type=1;c.cutoff=8192;c.resonance=32767;
 etch_native_midi(&s,0x90,60,120);
 etch_native_render(&s,&c,0,pcm,128);
 int32_t smooth_cutoff=0,smooth_resonance=0;
 etch_native_filter_ramp_state(&smooth_cutoff,&smooth_resonance);
 assert(smooth_cutoff>0&&smooth_cutoff<8192);
 assert(smooth_resonance>0&&smooth_resonance<32767);
 uint32_t before=(uint32_t)smooth_cutoff;
 c.cutoff=0;c.resonance=0;
 etch_native_render(&s,&c,0,pcm,1);
 etch_native_filter_ramp_state(&smooth_cutoff,&smooth_resonance);
 assert((uint32_t)smooth_cutoff<before && smooth_cutoff>=0);
 assert(smooth_resonance>=0);
 /* Stress non-overshooting control ramps and both stereo output paths. */
 for(int k=0;k<96;k++){
  c.cutoff=(k&1)?8192:0;c.resonance=(k&1)?32767:0;
  etch_native_render_stereo(&s,&c,0,pcm,128);
  etch_native_filter_ramp_state(&smooth_cutoff,&smooth_resonance);
  assert(smooth_cutoff>=0&&smooth_cutoff<=8192);
  assert(smooth_resonance>=0&&smooth_resonance<=32767);
  for(int i=0;i<256;i++)assert(pcm[i]<32767&&pcm[i]>-32768);
 }
 /* Dense chords remain below full-scale clipping, across FX processing. */
 init();c.filter_type=1;c.cutoff=7500;c.resonance=28000;
 c.chorus=18000;c.delay=10000;c.reverb=12000;
 for(int n=0;n<8;n++)etch_native_midi(&s,0x90,(uint8_t)(45+n*4),120);
 uint32_t hits=0,peak=0;
 for(int k=0;k<80;k++){
  etch_native_render_stereo(&s,&c,0,pcm,128);
  for(int i=0;i<256;i++){
   uint32_t mag=(uint32_t)abs((int)pcm[i]);
   if(mag>=32767u)hits++;
   if(mag>peak)peak=mag;
  }
 }
 assert(hits==0u);
 printf("V1.21 audio: all 5 mip transitions monotonic; cutoff/resonance ramps bounded; 8 voices + FX peak %u, no full-scale clips\\n",peak);
 return 0;
}
