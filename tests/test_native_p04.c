/* SPDX-License-Identifier: GPL-3.0-only */
/* PJT10: lossless bounded archive, exact preflight and impossible-case safety. */
#include "../firmware/fm1-native/native_project_codec.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct etch_wave_library waves,outwaves;
static struct etch_native_pattern sequence,loaded,small_pattern;
static struct etch_project_settings settings,outsettings;
static uint8_t bank[ETCH_PROJECT_MAX_BYTES],extended[100000];
static uint32_t seed=0x9e3779b9u;
static uint32_t rng(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static uint32_t encode_check(void){
 uint32_t required=0,used=0;
 assert(etch_project_measure(&waves,&sequence,&settings,&required)==0);
 assert(etch_project_encode(&waves,&sequence,&settings,bank,sizeof(bank),&used)==0);
 assert(required==used);
 assert(bank[0]=='P'&&bank[1]=='J'&&bank[2]=='T'&&bank[3]=='<');
 assert(etch_project_decode(bank,used,&outwaves,&loaded,&outsettings)==0);
 assert(outsettings.source_version==12);
 assert(outsettings.patterns.used==settings.patterns.used);
 assert(memcmp(outsettings.patterns.data,settings.patterns.data,settings.patterns.used)==0);
 assert(memcmp(outsettings.patterns.lengths,settings.patterns.lengths,sizeof(settings.patterns.lengths))==0);
 assert(memcmp(&sequence,&loaded,sizeof(sequence))==0);
 return used;
}
int main(void){
 settings.seq_length=256;settings.seq_gate=75;settings.tempo=120;etch_fx_defaults(&settings.global_fx);
 waves.valid[0]=waves.valid[1]=0xffffffffu;
 for(unsigned i=0;i<64;i++)for(unsigned j=0;j<128;j++)waves.slot[i][j]=(int16_t)((int)i*77+(int)j*4-2300);
 /* Fill a high-overlap pattern archive. Exact V9 raw cost is archive.used;
  * V10 compression is deterministic even with >10 KB of archived patterns. */
 for(unsigned slot=0;slot<20;slot++){
  etch_pattern_clear(&small_pattern);
  for(unsigned st=0;st<25;st++){
   unsigned step=(st*7u)%256;
   for(unsigned n=0;n<3;n++)assert(etch_seq_put(&small_pattern,step,(uint8_t)(50+n),100,5));
   small_pattern.steps[step].lock_mask=3;
   small_pattern.steps[step].locks[0]=1024;
   small_pattern.steps[step].locks[1]=2048;
  }
  assert(etch_pattern_bank_capture(&settings.patterns,(uint8_t)slot,&small_pattern)==0);
 }
 assert(settings.patterns.used>7000);
 uint32_t used=encode_check();
 /* Decoding a truncated, corrupted or length-forged compressed blob fails safely. */
 assert(etch_project_decode(bank,used-1,&outwaves,&loaded,&outsettings)!=0);
 uint32_t section=0;
 for(uint32_t at=0;at+70<used;at++){
  if(bank[at]==(uint8_t)settings.patterns.used && bank[at+1]==(uint8_t)(settings.patterns.used>>8) &&
     memcmp(bank+at+2,settings.patterns.lengths,64)==0){section=at+66;break;}
 }
 assert(section&&bank[section]==1); /* compressed archive mode */
 uint32_t archive_packed=(uint32_t)bank[section+1]+((uint32_t)bank[section+2]<<8);
 assert(archive_packed<settings.patterns.used);
 uint8_t saved_tag=bank[section+3],saved_dist=bank[section+4];
 bank[section+3]=0xc0;bank[section+4]=0; /* invalid backref distance zero */
 assert(etch_project_decode(bank,used,&outwaves,&loaded,&outsettings)!=0);
 bank[section+3]=saved_tag;bank[section+4]=saved_dist;
 uint8_t size_lo=bank[section+1],size_hi=bank[section+2];
 bank[section+1]=255;bank[section+2]=255;
 assert(etch_project_decode(bank,used,&outwaves,&loaded,&outsettings)!=0);
 bank[section+1]=size_lo;bank[section+2]=size_hi;
 assert(etch_project_decode(bank,used,&outwaves,&loaded,&outsettings)==0);
 /* A full 256x8+6-lock pattern must round-trip with compressed archives. */
 for(unsigned step=0;step<256;step++){
  for(unsigned n=0;n<8;n++)assert(etch_seq_put(&sequence,step,(uint8_t)(48+n),100,2));
  sequence.steps[step].lock_mask=63;
  for(unsigned k=0;k<6;k++)sequence.steps[step].locks[k]=(uint16_t)(1500+500*k);
 }
 uint32_t budget=0;
 assert(etch_project_measure(&waves,&sequence,&settings,&budget)==0);
 int possible=budget<=ETCH_PROJECT_MAX_BYTES;
 if(possible)encode_check();
 else {uint32_t ignored=123;assert(etch_project_encode(&waves,&sequence,&settings,bank,sizeof(bank),&ignored)==-2);}
 /* Worst case is provably not guaranteed by a 20,224-byte bank. Measure exact
  * size using the SAME codec, and reject on-device without data loss. */
 for(unsigned i=0;i<64;i++)for(unsigned j=0;j<128;j++)waves.slot[i][j]=(int16_t)(rng()>>16);
 assert(etch_project_measure(&waves,&sequence,&settings,&budget)==0);
 assert(budget>ETCH_PROJECT_MAX_BYTES);
 uint32_t ignored=0;
 assert(etch_project_encode(&waves,&sequence,&settings,bank,sizeof(bank),&ignored)==-2);
 assert(etch_project_encode(&waves,&sequence,&settings,extended,sizeof(extended),&ignored)==0);
 assert(ignored==budget);
 printf("PASS P04 PJT10: pattern archive raw=%u compact=%u; total project=%u; max-stress requires=%u > bank=%u; exact preflight, invalid tokens rejected\n",
 settings.patterns.used,archive_packed,used,budget,ETCH_PROJECT_MAX_BYTES);
}
