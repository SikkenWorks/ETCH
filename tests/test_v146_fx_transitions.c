/* GPL-3.0-only. V1.46 FX golden comparison against UNMODIFIED V1.45.
 * Compile this same source with V1.45 and V1.46 native_fx.c, diff lines.
 * Tests continuous DSP state and PCM across on/off transitions, not just dry. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "native_fx.h"

static struct etch_native_fx fx;
static const uint32_t v145_golden[10]={
 0x9cd3bd34u,0xdb97b259u,0xaa5e0008u,0x10582ef2u,0x6afbe20bu,
 0xbd31190bu,0x335ed64du,0xbe1cae60u,0xafc5a723u,0x24b8da15u
};
static uint32_t checksum=2166136261u;
static void hash16(uint16_t x){checksum=(checksum^(x&255u))*16777619u; checksum=(checksum^(x>>8))*16777619u;}
static void hash32(uint32_t x){hash16((uint16_t)x); hash16((uint16_t)(x>>16));}
static void run_case(unsigned scenario){
 struct etch_fx_settings cfg;
 etch_fx_reset(&fx);
 etch_fx_defaults(&cfg);
 checksum=2166136261u;
 uint32_t seed=0x98651234u;
 for(unsigned i=0;i<16000u;i++){
  uint32_t segment=i/1000u;
  uint16_t chorus=0,delay=0,reverb=0;
  if(scenario==0){chorus=(segment&1u)?0u:17000u; delay=9000u; reverb=11000u;}
  if(scenario==1){chorus=0u;delay=(segment&1u)?0u:12000u;reverb=18000u;}
  if(scenario==2){chorus=0u;delay=10000u;reverb=(segment&1u)?0u:22000u;}
  if(scenario==3){chorus=(segment&1u)?4000u:16000u;delay=0u;reverb=0u;}
  if(scenario==4){chorus=0u;delay=0u;reverb=0u;cfg.comp_ratio=18000u;cfg.comp_threshold=11000u;cfg.eq_low=25000u;}
  if(scenario==5){chorus=0u;delay=0u;reverb=0u;cfg.flanger_mix=(segment&1u)?0u:17000u;cfg.folder_mix=11000u;}
  if(scenario==6){chorus=0u;delay=0u;reverb=0u;cfg.routing=1;cfg.ring_mix=12000u;}
  if(scenario==7){chorus=0u;delay=0u;reverb=0u; if((segment&3u)==0u)cfg.phaser_mix=8000u;else cfg.phaser_mix=0u;}
  if(scenario==8){chorus=0u;delay=0u;reverb=(segment&1u)?100u:0u;cfg.routing=1u;}
  if(scenario==9){chorus=0u;delay=0u;reverb=0u; cfg.reverb_decay=(uint16_t)((segment*2000u)%32768u);}
  seed=1664525u*seed+1013904223u;
  int16_t input=(int16_t)(seed>>16);
  int16_t l=0,r=0;
  etch_fx_process_stereo_ex(&fx,input,chorus,delay,reverb,&cfg,&l,&r);
  hash16((uint16_t)l);hash16((uint16_t)r);
 }
 /* Hash state that matters when a user re-enables an effect later. */
 hash32(fx.delay_pos);hash32(fx.chorus_pos);hash32(fx.reverb_pos);
 hash32(fx.chorus_phase);hash32((uint32_t)fx.space_damp_state);
 hash32((uint32_t)fx.reverb_mix_smooth);
 hash32((uint32_t)fx.eq_lp_low);hash32((uint32_t)fx.eq_lp_high);
 for(unsigned i=0;i<ETCH_DELAY_SIZE;i+=29u)hash16((uint16_t)fx.delay[i]);
 for(unsigned i=0;i<ETCH_REVERB_SIZE;i+=11u)hash16((uint16_t)fx.reverb[i]);
 for(unsigned i=0;i<ETCH_CHORUS_SIZE;i+=7u)hash16((uint16_t)fx.chorus[i]);
 if(checksum!=v145_golden[scenario]){
  fprintf(stderr,"FAIL case=%u expected=%08x actual=%08x\n",scenario,v145_golden[scenario],checksum);
  __builtin_trap();
 }
 printf("PASS case=%u pcm+state=%08x\n",scenario,checksum);
}
int main(void){for(unsigned i=0;i<10u;i++)run_case(i);return 0;}
