/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.25: Living Waves / PJT23, host-only, no claims of flash safety.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_native_audio a;
static struct etch_native_controls c;
static struct etch_wave_library l,l2;
static struct etch_native_pattern p,p2;
static struct etch_project_settings cfg,cfg2;
static uint16_t riffs[ETCH_PATCH_SLOTS],riffs2[ETCH_PATCH_SLOTS];
static uint8_t wire[ETCH_PROJECT_MAX_BYTES];
static int16_t frames[128];
static uint64_t render(uint8_t depth,uint8_t rate,uint8_t color,uint8_t vary){
 etch_native_reset(&a);memset(&c,0,sizeof(c));etch_fx_defaults(&c.fx_settings);
 c.gain=32767;c.tempo=120;c.env_enabled=1;c.morph=16384;
 c.attack_step=c.decay_step=0x7fff0000u;c.sustain_level=32767;c.release_step=0x20000u;
 c.living=(struct etch_living_settings){depth,rate,color,vary};
 etch_native_midi(&a,0x90,38,127);etch_native_midi(&a,0x90,47,99);
 etch_native_midi(&a,0x90,56,80);etch_native_midi(&a,0x90,65,71);
 uint64_t hash=1469598103934665603ull;
 for(int b=0;b<240;b++){
  etch_native_render(&a,&c,0,frames,128);
  if(b<4)continue;
  for(int i=0;i<128;i++)hash=(hash^(uint16_t)frames[i])*1099511628211ull;
 }
 return hash;
}
int main(void){
 assert(etch_living_period(100u)<etch_living_period(0u));
 struct etch_living_settings invalid={101u,40u,40u,40u};
 assert(!etch_living_valid(&invalid));
 assert(etch_living_weight(500u,42u,127u,&invalid)==0u);
 struct etch_living_settings d={80u,80u,0u,0u};
 assert(etch_living_weight(0u,2u,127u,&d)==0u);
 assert(etch_living_weight(1500u,2u,127u,&d)>0u);
 assert(etch_living_mix(1000,700,0,0)==1000);
 assert(etch_living_mix(1000,700,32767,0)==700);
 assert(etch_living_mix(1000,700,32767,100)>1000);
 uint64_t dry=render(0,0,0,0);
 assert(dry==render(0,100,100,100)); /* bit-exact bypass */
 uint64_t dark=render(100,100,0,0);
 assert(dark!=dry&&dark==render(100,100,0,0));
 uint64_t bright=render(100,100,100,100);
 assert(bright!=dry&&bright!=dark&&bright==render(100,100,100,100));
 assert(render(75,30,0,0)!=render(75,100,0,0));
 /* PJT23 roundtrip, retention of RIFF seeds and PJT22 migration. */
 etch_project_legacy(&l,&p,&cfg);cfg.source_version=23;
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;
 cfg.organic=(struct etch_organic_settings){10,20,30,40};
 cfg.living=(struct etch_living_settings){59,63,42,87};
 etch_project_bind_riffs(&cfg,riffs);riffs[3]=0x3344u;
 uint32_t used=0;
 assert(!etch_project_encode(&l,&p,&cfg,wire,sizeof(wire),&used));
 assert(!memcmp(wire,"PJTG",4));
 etch_project_bind_riffs(&cfg2,riffs2);
 assert(!etch_project_decode(wire,used,&l2,&p2,&cfg2));
 assert(cfg2.source_version==23&&riffs2[3]==0x3344u);
 assert(!memcmp(&cfg2.organic,&cfg.organic,sizeof(cfg.organic)));
 assert(!memcmp(&cfg2.living,&cfg.living,sizeof(cfg.living)));
 assert(etch_project_decode(wire,used-1u,&l2,&p2,&cfg2));
 cfg.source_version=22;
 assert(!etch_project_encode(&l,&p,&cfg,wire,sizeof(wire),&used));
 assert(!etch_project_decode(wire,used,&l2,&p2,&cfg2));
 assert(cfg2.source_version==22&&cfg2.living.depth==0&&cfg2.living.color==0&&cfg2.organic.phase==30);
 puts("PASS Living Waves V1.25: per-voice motion, dark/bright, deterministic, bit-exact bypass, PJT23/PJT22");
}
