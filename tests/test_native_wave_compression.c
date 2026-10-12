/* SPDX-License-Identifier: GPL-3.0-only: V9 codec lossless and capacity boundaries. */
#include "../firmware/fm1-native/native_project_codec.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
static struct etch_wave_library a,b;
static struct etch_native_pattern p,q;
static struct etch_project_settings settings,readback;
static uint8_t stream[ETCH_PROJECT_MAX_BYTES];
static uint32_t seed=0x1879ABCDu;
static uint32_t rnd(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static uint32_t encode(void){uint32_t bytes=0;assert(!etch_project_encode(&a,&p,&settings,stream,sizeof(stream),&bytes));return bytes;}
static void roundtrip(void){uint32_t n=encode();assert(!etch_project_decode(stream,n,&b,&q,&readback));assert(readback.source_version==12);for(unsigned i=0;i<64;i++)assert(!memcmp(a.slot[i],b.slot[i],256));}
int main(void){
 a.valid[0]=a.valid[1]=0xffffffffu;
 settings.seq_length=256;settings.seq_gate=75;settings.tempo=120;etch_fx_defaults(&settings.global_fx);
 /* 64 slowly changing hand-drawn waves: the new compressor must recover every exact sample. */
 for(unsigned i=0;i<64;i++)for(unsigned j=0;j<128;j++)a.slot[i][j]=(int16_t)((int)i*87+(int)j*8-3200);
 roundtrip();uint32_t packed=encode();
 assert(packed<14000);
 /* Full polyphonic 256-step density combines with the waveform library. */
 for(unsigned step=0;step<256;step++){
  for(unsigned note=0;note<8;note++)assert(etch_seq_put(&p,step,48+note,100,2));
  p.steps[step].lock_mask=63;
 }
 uint32_t dense=0;int fit=etch_project_encode(&a,&p,&settings,stream,sizeof(stream),&dense);
 assert(fit==0);assert(!etch_project_decode(stream,dense,&b,&q,&readback));assert(!memcmp(&p,&q,sizeof(p)));
 /* Incompressible waveforms cannot be silently truncated; report capacity error. */
 for(unsigned i=0;i<64;i++)for(unsigned j=0;j<128;j++)a.slot[i][j]=(int16_t)(rnd()>>16);
 uint32_t ignored=0;assert(etch_project_encode(&a,&p,&settings,stream,sizeof(stream),&ignored)==-2);
 etch_pattern_clear(&p);
 roundtrip();
 /* Byte-identical values after random signed 16-bit input, including +/-32768. */
 a.slot[0][0]=-32768;a.slot[0][1]=32767;a.slot[0][2]=-32768;
 roundtrip();
 printf("PASS V10: 64 lossless waves, dense pattern capacity %s, random raw fallback, signed-extrema, fail-closed over-capacity (base %u bytes)\n",fit==0?"FITS":"FULL",packed);
}
