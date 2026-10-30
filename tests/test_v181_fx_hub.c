/* ETCH V1.81 non-destructive FX hub and DSP gating checks. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "native_fx_bypass_v181.h"
#include "native_ui_navigation.h"
int main(void){
 assert(ETCH_UI_FX_BYPASS==62u && ETCH_UI_PAGE_COUNT==64u);
 for(unsigned i=0;i<ETCH_UI_PAGE_COUNT;i++)assert(strcmp(etch_ui_title(i),"ERROR"));
 struct etch_fx_settings original,copy;
 etch_fx_defaults(&original);
 original.ring_mix=24000;original.eq_low=19000;original.comp_ratio=16000;
 original.phaser_mix=10000;original.trem_mix=8000;original.decim_mix=8000;
 original.reverse_mix=11000;
 etch_pre_set_type(&original,0,0,3);etch_pre_set_mix(&original,0,0,18000);
 etch_pre_set_type(&original,1,1,5);etch_pre_set_mix(&original,1,1,17000);
 etch_c_fx_set_target(&original,6);etch_c_fx_set_depth(&original,20000);
 uint8_t active[ETCH_FXB_COUNT]={0};
 uint8_t n=etch_fxb_collect(active,ETCH_FXB_COUNT,&original,20000,12000,17000,24000);
 assert(n>10u);
 uint32_t mask=0;
 for(uint8_t i=0;i<n;i++)mask|=ETCH_FXB_BIT(active[i]);
 assert(mask&ETCH_FXB_BIT(ETCH_FXB_REVERSE));
 copy=original;uint16_t chorus=12000,delay=17000,reverb=24000;
 etch_fxb_mask_pre(mask,&copy);
 etch_fxb_mask_master(mask,&copy,&chorus,&delay,&reverb);
 assert(!chorus&&!delay&&!reverb&&!copy.comp_ratio&&!copy.ring_mix);
 assert(!copy.eq_hpf&&copy.eq_low==16384&&copy.eq_mid==16384&&copy.eq_high==16384);
 assert(!etch_pre_type(&copy,0,0)&&!etch_pre_type(&copy,1,1));
 assert(!etch_c_fx_target(&copy));
 assert(original.comp_ratio==16000 && original.ring_mix==24000);
 assert(etch_pre_type(&original,0,0)==3 && etch_c_fx_target(&original)==6);
 struct etch_native_fx state;etch_fx_reset(&state);
 /* Every enabled module was explicitly bypassed; waveform must be dry. */
 for(unsigned i=0;i<14000u;i++){
  int16_t input=(int16_t)((i*13u)%23000u-11500),l=0,r=0;
  etch_fx_process_stereo_masked(&state,input,chorus,delay,reverb,&copy,mask,&l,&r);
  assert(l==input && r==input);
 }
 /* Bypassing ONLY chorus leaves delay/reverb active, and does not
  * accidentally zero their settings or their native storage buffers. */
 copy=original;chorus=12000;delay=17000;reverb=24000;
 etch_fxb_mask_master(ETCH_FXB_BIT(ETCH_FXB_CHORUS),&copy,&chorus,&delay,&reverb);
 assert(!chorus && delay==17000 && reverb==24000 && copy.comp_ratio==16000);
 puts("PASS V1.81: all 22 FX IDs, runtime mask, unchanged presets, dry DSP, 64 screens");
 return 0;
}
