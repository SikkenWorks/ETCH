/* SPDX-License-Identifier: GPL-3.0-only
 * Seven-plus voices render identically in POLY with LIVING preset settings
 * regardless of values. Switching to MONO re-enables LIVING audibly.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_tables.h"
static struct etch_native_audio a,b;
static struct etch_native_controls x,y;
static struct etch_native_bank bank;
static int16_t abuf[256],bbuf[256];
static void defaults(struct etch_native_controls *c,uint8_t mono){
 memset(c,0,sizeof(*c));etch_fx_defaults(&c->fx_settings);
 c->bank=&bank;c->gain=24000;c->tempo=120;c->morph=16000;
 c->env_enabled=1;c->attack_step=0x7fff0000u;c->decay_step=0x7fff0000u;
 c->sustain_level=26000;c->release_step=0x02000000u;c->mono_mode=mono;
}
int main(void){
 for(int w=0;w<3;w++)for(int m=0;m<6;m++)for(int k=0;k<512;k++)
  bank.table[w][m][k]=etch_tables[w][m][k];
 defaults(&x,0); y=x;
 y.living.depth=100;y.living.rate=100;y.living.color=0;y.living.vary=100;
 etch_native_reset(&a);etch_native_reset(&b);
 for(unsigned i=0;i<8;i++){
  etch_native_midi(&a,0x90,(uint8_t)(48+i*3),120);
  etch_native_midi(&b,0x90,(uint8_t)(48+i*3),120);
 }
 unsigned long energy=0;
 for(unsigned block=0;block<40;block++){
  etch_native_render_stereo(&a,&x,0,abuf,128);
  etch_native_render_stereo(&b,&y,0,bbuf,128);
  assert(memcmp(abuf,bbuf,sizeof(abuf))==0);
  for(unsigned i=0;i<256;i++){energy+=(uint16_t)(abuf[i]<0?-abuf[i]:abuf[i]);}
 }
 assert(energy>0);
 /* Same preset values must be preserved when changing mode; MONO should
  * react to high-depth settings once only a single note is active. */
 etch_native_reset(&a);etch_native_reset(&b);
 defaults(&x,1);y=x;
 y.living.depth=100;y.living.rate=100;y.living.color=0;y.living.vary=100;
 etch_native_midi(&a,0x90,57,127);etch_native_midi(&b,0x90,57,127);
 unsigned long unequal=0;
 for(unsigned block=0;block<120;block++){
  etch_native_render_stereo(&a,&x,0,abuf,128);
  etch_native_render_stereo(&b,&y,0,bbuf,128);
  for(unsigned i=0;i<256;i++)if(abuf[i]!=bbuf[i])unequal++;
 }
 assert(unequal>100);
 puts("PASS V1.33 8-voice POLY living bypass bit-exact; MONO re-enables harmonic motion");
}
