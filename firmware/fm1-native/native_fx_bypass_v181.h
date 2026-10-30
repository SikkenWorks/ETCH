/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.81 temporary, non-destructive per-effect bypass.
 * Bits are runtime-only. PJT31, patches and waveforms remain unmodified.
 */
#pragma once
#include <stdint.h>
#include "native_fx.h"

enum etch_fx_bypass_id {
 ETCH_FXB_DRIVE,ETCH_FXB_CHORUS,ETCH_FXB_DELAY,ETCH_FXB_REVERB,
 ETCH_FXB_RING,ETCH_FXB_EQ,ETCH_FXB_COMP,ETCH_FXB_TREM,
 ETCH_FXB_PHASER,ETCH_FXB_FLANGER,ETCH_FXB_DECIM,ETCH_FXB_FOLDER,
 ETCH_FXB_TAPE,ETCH_FXB_REVERSE,ETCH_FXB_VIBRATO,ETCH_FXB_ROTARY,
 ETCH_FXB_WAH,ETCH_FXB_A1,ETCH_FXB_A2,ETCH_FXB_B1,ETCH_FXB_B2,
 ETCH_FXB_CMOD,ETCH_FXB_COUNT
};
#define ETCH_FXB_BIT(id) (1u<<(id))
#define ETCH_FXB_ALL ((1u<<ETCH_FXB_COUNT)-1u)
static inline const char *etch_fxb_name(uint8_t id){
 static const char *const names[ETCH_FXB_COUNT]={
  "DRIVE","CHORUS","DELAY","REVERB","RING MOD","CHANNEL EQ",
  "COMPRESSOR","TREMOLO","PHASER","FLANGER","DECIMATOR",
  "WAVEFOLDER","TAPE","REVERSE","VIBRATO","ROTARY","AUTO-WAH",
  "WAVE A FX 1","WAVE A FX 2","WAVE B FX 1","WAVE B FX 2","WAVE C FX"};
 return id<ETCH_FXB_COUNT?names[id]:"UNKNOWN";
}
static inline uint8_t etch_fxb_configured(uint8_t id,const struct etch_fx_settings *c,
 uint16_t drive,uint16_t chorus,uint16_t delay,uint16_t reverb){
 if(!c)return 0u;
 switch(id){
 case ETCH_FXB_DRIVE:return drive!=0u;
 case ETCH_FXB_CHORUS:return chorus!=0u;
 case ETCH_FXB_DELAY:return delay!=0u;
 case ETCH_FXB_REVERB:return reverb!=0u;
 case ETCH_FXB_RING:return c->ring_mix!=0u;
 case ETCH_FXB_EQ:return (uint8_t)(c->eq_hpf||c->eq_low!=16384u||c->eq_mid!=16384u||c->eq_high!=16384u);
 case ETCH_FXB_COMP:return c->comp_ratio!=0u;
 case ETCH_FXB_TREM:return c->trem_mix!=0u;
 case ETCH_FXB_PHASER:return c->phaser_mix!=0u;
 case ETCH_FXB_FLANGER:return c->flanger_mix!=0u;
 case ETCH_FXB_DECIM:return c->decim_mix!=0u;
 case ETCH_FXB_FOLDER:return c->folder_mix!=0u;
 case ETCH_FXB_TAPE:return c->tape_mix!=0u;
 case ETCH_FXB_REVERSE:return c->reverse_mix!=0u;
 case ETCH_FXB_VIBRATO:return c->vibrato_mix!=0u;
 case ETCH_FXB_ROTARY:return c->rotary_mix!=0u;
 case ETCH_FXB_WAH:return c->wah_mix!=0u;
 case ETCH_FXB_A1:return (uint8_t)(etch_pre_type(c,0u,0u)!=0u && etch_pre_mix(c,0u,0u)!=0u);
 case ETCH_FXB_A2:return (uint8_t)(etch_pre_type(c,0u,1u)!=0u && etch_pre_mix(c,0u,1u)!=0u);
 case ETCH_FXB_B1:return (uint8_t)(etch_pre_type(c,1u,0u)!=0u && etch_pre_mix(c,1u,0u)!=0u);
 case ETCH_FXB_B2:return (uint8_t)(etch_pre_type(c,1u,1u)!=0u && etch_pre_mix(c,1u,1u)!=0u);
 case ETCH_FXB_CMOD:return (uint8_t)(etch_c_fx_target(c)!=0u && etch_c_fx_depth(c)!=0u);
 default:return 0u;
 }
}
/* Call after automation/parameter locks, before post-mix FX. Never rewrite
 * the stored patch or force any flash operations. The FX processor receives
 * the mask too, to skip expensive time-domain algorithms, not just wet mix. */
static inline void etch_fxb_mask_master(uint32_t bypass,struct etch_fx_settings *c,
 uint16_t *chorus,uint16_t *delay,uint16_t *reverb){
 if(!bypass||!c)return;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_CHORUS))*chorus=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_DELAY))*delay=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_REVERB))*reverb=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_RING))c->ring_mix=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_EQ)){c->eq_low=16384u;c->eq_mid=16384u;c->eq_high=16384u;c->eq_hpf=0u;}
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_COMP))c->comp_ratio=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_TREM))c->trem_mix=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_PHASER))c->phaser_mix=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_FLANGER))c->flanger_mix=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_DECIM))c->decim_mix=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_FOLDER))c->folder_mix=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_TAPE))c->tape_mix=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_REVERSE))c->reverse_mix=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_VIBRATO))c->vibrato_mix=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_ROTARY))c->rotary_mix=0u;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_WAH))c->wah_mix=0u;
}
static inline void etch_fxb_mask_pre(uint32_t bypass,struct etch_fx_settings *c){
 if(!bypass||!c)return;
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_A1))etch_pre_set_type(c,0u,0u,0u);
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_A2))etch_pre_set_type(c,0u,1u,0u);
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_B1))etch_pre_set_type(c,1u,0u,0u);
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_B2))etch_pre_set_type(c,1u,1u,0u);
 if(bypass&ETCH_FXB_BIT(ETCH_FXB_CMOD))etch_c_fx_set_target(c,0u);
}
static inline uint8_t etch_fxb_collect(uint8_t *out,uint8_t cap,const struct etch_fx_settings *c,
 uint16_t drive,uint16_t chorus,uint16_t delay,uint16_t reverb){
 uint8_t count=0u;
 for(uint8_t i=0;i<ETCH_FXB_COUNT;i++)if(etch_fxb_configured(i,c,drive,chorus,delay,reverb)){
  if(count<cap)out[count]=i;
  count++;
 }
 return count;
}
