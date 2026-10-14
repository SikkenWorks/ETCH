/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.23 Organic: independent per-note evolution and PJT22 migration.
 * Host unit test, not a hardware timing or flash-safety certification.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_native_audio a;
static struct etch_native_controls c;
static struct etch_wave_library lib,lib2;
static struct etch_native_pattern pat,pat2;
static struct etch_project_settings cfg,cfg2;
static uint16_t riffs[ETCH_PATCH_SLOTS],riffs2[ETCH_PATCH_SLOTS];
static uint8_t packed[ETCH_PROJECT_MAX_BYTES];
static int16_t pcm[128];
static uint64_t render(uint8_t d,uint8_t rate,uint8_t phase,uint8_t variance){
 etch_native_reset(&a);memset(&c,0,sizeof(c));etch_fx_defaults(&c.fx_settings);
 c.gain=32767;c.tempo=120;c.env_enabled=1;c.mono_mode=1;
 c.attack_step=c.decay_step=0x7fff0000u;c.sustain_level=32767;c.release_step=0x20000u;
 c.organic=(struct etch_organic_settings){d,rate,phase,variance};
 etch_native_midi(&a,0x90,48,120);etch_native_midi(&a,0x90,55,120);
 etch_native_midi(&a,0x90,60,120);etch_native_midi(&a,0x90,64,120);
 uint64_t hash=1469598103934665603ull;
 for(int b=0;b<90;b++){
  etch_native_render(&a,&c,0,pcm,128);
  if(b<4)continue;
  for(int i=0;i<128;i++)hash=(hash^(uint16_t)pcm[i])*1099511628211ull;
 }
 return hash;
}
int main(void){
 assert(etch_organic_interval(0)>etch_organic_interval(100));
 assert(etch_organic_target(22,100)<=454&&etch_organic_target(22,100)>=-454);
 assert(etch_organic_initial_phase(13,0)==0u);
 assert(etch_organic_pitch_inc(1000000,0)==1000000);
 struct etch_organic_settings invalid={101,0,0,0};assert(!etch_organic_valid(&invalid));
 uint64_t dry=render(0,0,0,0);
 assert(dry==render(0,0,0,0));
 uint64_t organic=render(65,40,40,45);
 assert(organic==render(65,40,40,45));
 assert(organic!=dry);
 assert(render(0,0,0,0)!=render(0,0,100,0));
 assert(render(0,0,0,0)!=render(0,0,0,100));
 etch_project_legacy(&lib,&pat,&cfg);cfg.source_version=22;
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;
 cfg.organic=(struct etch_organic_settings){41,27,13,9};
 etch_project_bind_riffs(&cfg,riffs);riffs[2]=0x1234u;
 uint32_t used=0;
 assert(etch_project_encode(&lib,&pat,&cfg,packed,sizeof(packed),&used)==0);
 assert(!memcmp(packed,"PJTF",4));
 etch_project_bind_riffs(&cfg2,riffs2);
 assert(etch_project_decode(packed,used,&lib2,&pat2,&cfg2)==0);
 assert(cfg2.source_version==22&&riffs2[2]==0x1234);
 assert(!memcmp(&cfg2.organic,&cfg.organic,sizeof(cfg.organic)));
 assert(etch_project_decode(packed,used-1,&lib2,&pat2,&cfg2)!=0);
 /* Old projects must decode with neutral Organic settings. */
 cfg.source_version=21;
 assert(etch_project_encode(&lib,&pat,&cfg,packed,sizeof(packed),&used)==0);
 etch_project_bind_riffs(&cfg2,riffs2);
 assert(etch_project_decode(packed,used,&lib2,&pat2,&cfg2)==0);
 assert(cfg2.source_version==21&&cfg2.organic.drift==0&&cfg2.organic.phase==0&&cfg2.organic.variance==0);
 puts("PASS V1.35 organic MONO: off, drift, phase, variance, priority stack, PJT22 and PJT21 migration");
}
