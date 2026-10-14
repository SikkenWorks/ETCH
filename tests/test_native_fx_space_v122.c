/* SPDX-License-Identifier: GPL-3.0-only
 * V1.22 SPACE 2.0 regression: dry exactness, bounded diffusion and stereo.
 * Host only; does not imply CPU timing or hardware DAC qualification.
 */
#include "../firmware/fm1-native/native_fx.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static struct etch_native_fx fx;
static int32_t abs32(int32_t v){return v<0?-v:v;}
int main(void){
 struct etch_fx_settings cfg;etch_fx_defaults(&cfg);
 int16_t l=0,r=0;
 /* A fresh instance must reproduce the unprocessed 16-bit input exactly. */
 etch_fx_reset(&fx);
 for(unsigned i=0;i<6000;i++){
  int16_t x=(int16_t)((i*131u%32001u)-16000);
  etch_fx_process_stereo_ex(&fx,x,0,0,0,&cfg,&l,&r);
  assert(l==x&&r==x);
 }
 /* Loud, decaying pulse: check early returns, late tail and stereo difference. */
 etch_fx_reset(&fx);cfg.reverb_size=26000;cfg.reverb_decay=29000;
 int64_t early=0,late=0,side=0,peak=0;
 for(unsigned i=0;i<22000;i++){
  int16_t in=(i<1200 && (i%73u)<36u)?(int16_t)14000:(int16_t)0;
  etch_fx_process_stereo_ex(&fx,in,0,0,32000,&cfg,&l,&r);
  assert(l<32767&&l>-32768&&r<32767&&r>-32768);
  if(i>200&&i<2700)early+=abs32(l);
  if(i>3000&&i<12000)late+=abs32(l);
  if(i>600&&i<15000)side+=abs32((int32_t)l-r);
  if(abs32(l)>peak)peak=abs32(l);
 }
 assert(early>30000 && late>3000 && side>5000 && peak<32767);
 /* Sweep SPACE size + decay under loud chords; prevent overflows or silence. */
 for(unsigned i=0;i<30000;i++){
  cfg.reverb_size=(uint16_t)((i/107u%2u)?32767u:0u);
  cfg.reverb_decay=(uint16_t)((i/113u%2u)?32767u:16000u);
  int16_t x=(int16_t)((i%101u)<51u?25000:-25000);
  etch_fx_process_stereo_ex(&fx,x,24000,17000,26000,&cfg,&l,&r);
  assert(l<32767&&l>-32768&&r<32767&&r>-32768);
 }
 /* Fractional, antiphasic chorus must be stereo and audible. */
 etch_fx_reset(&fx);etch_fx_defaults(&cfg);cfg.chorus_depth=30000;
 int64_t chorus_diff=0;
 for(unsigned i=0;i<17000;i++){
  int16_t x=(int16_t)((i%61u)<30u?9000:-9000);
  etch_fx_process_stereo_ex(&fx,x,25000,0,0,&cfg,&l,&r);
  if(i>2000)chorus_diff+=abs32((int32_t)l-r);
 }
 assert(chorus_diff>200000);
 /* No held stale tank audio may return after an exact dry bypass and flush. */
 etch_fx_defaults(&cfg);
 for(unsigned i=0;i<20000;i++){
  int16_t x=0;
  etch_fx_process_stereo_ex(&fx,x,0,0,0,&cfg,&l,&r);
  assert(l==0&&r==0);
 }
 assert(!fx.dry_flush_active);
 for(unsigned i=0;i<ETCH_REVERB_SIZE;i++)assert(fx.reverb[i]==0);
 etch_fx_process_stereo_ex(&fx,0,0,0,30000,&cfg,&l,&r);
 assert(l==0&&r==0);
 printf("PASS V1.22 SPACE 2.0: early=%lld late=%lld stereo=%lld chorus=%lld peak=%lld, dry/flush safe\n",(long long)early,(long long)late,(long long)side,(long long)chorus_diff,(long long)peak);
 return 0;
}
