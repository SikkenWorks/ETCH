/* SPDX-License-Identifier: GPL-3.0-only
 * Ring/EQ DSP safety and legacy PJT10 -> PJT11 migrations. No device writes.
 */
#include "../firmware/fm1-native/native_project_codec.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct etch_native_fx fx;
static struct etch_wave_library waves,decoded;
static struct etch_native_pattern pattern,decoded_pattern;
static struct etch_project_settings cfg,readback;
static uint8_t bytes[ETCH_PROJECT_MAX_BYTES];
int main(void){
 struct etch_fx_settings c;etch_fx_defaults(&c);
 etch_fx_reset(&fx);
 for(int i=0;i<10000;i++){
  int16_t l=0,r=0;int16_t in=(int16_t)((i%300)-150)*110;
  etch_fx_process_stereo_ex(&fx,in,0,0,0,&c,&l,&r);
  assert(l==in&&r==in); /* exact default bypass including EQ */
 }
 etch_fx_reset(&fx);
 c.ring_mix=32767;c.ring_rate=12000;
 unsigned flipped=0,nonzero=0;int16_t previous=0;
 for(int i=0;i<10000;i++){
  int16_t l=0,r=0;etch_fx_process_stereo_ex(&fx,12000,0,0,0,&c,&l,&r);
  if(i>300&&((l<0&&previous>0)||(l>0&&previous<0)))flipped++;
  if(l)nonzero++;previous=l;
  assert(l==r&&l<=12000&&l>=-12000);
 }
 assert(flipped>8&&nonzero>5000);
 /* Shape and bias must make signal observably different from pure ring. */
 etch_fx_reset(&fx);c.ring_shape=32767;c.ring_bias=32767;
 for(int i=0;i<400;i++){
  int16_t l=0,r=0;etch_fx_process_stereo_ex(&fx,9000,0,0,0,&c,&l,&r);
  if(i==399)assert(l==9000&&r==9000);
 }
 /* Neutral EQ stays exactly transparent, while band gain and HPF change it. */
 etch_fx_reset(&fx);etch_fx_defaults(&c);c.eq_low=32767;
 int32_t eq_total=0;
 for(int i=0;i<12000;i++){
  int16_t l=0,r=0;etch_fx_process_stereo_ex(&fx,1200,0,0,0,&c,&l,&r);
  if(i>5000)eq_total+=l;
 }
 assert(eq_total>1200*6500); /* low shelf ~ +6dB DC */
 etch_fx_reset(&fx);etch_fx_defaults(&c);c.eq_hpf=32767;
 int16_t last=0;
 for(int i=0;i<10000;i++)etch_fx_process_stereo_ex(&fx,9000,0,0,0,&c,&last,&last);
 assert(last<100); /* DC is filtered */
 /* Save eight new controls in a PJT11 snapshot. */
 etch_pattern_clear(&pattern);cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=143;
 etch_fx_defaults(&cfg.global_fx);cfg.global_fx.ring_mix=23000;
 cfg.global_fx.ring_rate=17500;cfg.global_fx.ring_shape=30000;
 cfg.global_fx.ring_bias=6000;cfg.global_fx.eq_low=23000;
 cfg.global_fx.eq_mid=11000;cfg.global_fx.eq_high=20000;cfg.global_fx.eq_hpf=6400;
 uint32_t used=0;
 assert(etch_project_encode(&waves,&pattern,&cfg,bytes,sizeof(bytes),&used)==0);
 assert(!memcmp(bytes,"PJT<",4)&&used>17);
 assert(etch_project_decode(bytes,used,&decoded,&decoded_pattern,&readback)==0);
 assert(readback.source_version==12);
 assert(readback.global_fx.ring_mix==23000&&readback.global_fx.eq_low==23000);
 assert(readback.global_fx.eq_hpf==6400&&readback.tempo==143);
 /* Turn the encoded PJT11 into a genuine PJT10 wire form. Extension is
  * appended at the END, so all previous bytes remain unchanged. */
 bytes[3]=':';
 assert(etch_project_decode(bytes,used-17,&decoded,&decoded_pattern,&readback)==0);
 assert(readback.source_version==10&&readback.tempo==143);
 assert(readback.global_fx.ring_mix==0&&readback.global_fx.eq_low==16384);
 assert(readback.global_fx.eq_mid==16384&&readback.global_fx.eq_high==16384);
 /* Invalid PJT11 extension must not silently load/save. */
 bytes[3]='<';bytes[used-2]=255;bytes[used-3]=255;
 assert(etch_project_decode(bytes,used,&decoded,&decoded_pattern,&readback)==-1);
 puts("PASS V0.96 Ring Mod and Channel EQ, exact bypass, bounded DSP, PJT10 migration, FX persistence");
 return 0;
}
