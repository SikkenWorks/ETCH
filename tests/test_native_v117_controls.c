#include <assert.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_live_controls_v117.h"
#include "../firmware/fm1-native/native_ui_wave.h"
static uint32_t footer(const struct etch_fx_settings *s, const struct etch_evolution_settings *e,uint8_t slot){
 const uint16_t keys[]={ETCH_V117_EXTRA_FOOTER_VALUES((*s),(*e),slot)};
 return etch_ui_values_fingerprint(keys,sizeof(keys)/sizeof(keys[0]));
}
int main(void){
 struct etch_fx_settings s={0};etch_fx_defaults(&s);
 struct etch_evolution_settings e={0,30,0,0};
 uint32_t old=footer(&s,&e,0);
 assert(etch_pre_type(&s,0,0)==0 && etch_pre_mix(&s,0,0)==0);
 /* K2: OFF -> RING, and auto-wet makes a new insert audible. */
 etch_v117_pre_turn(&s,0,0,1,0,0);
 assert(etch_pre_type(&s,0,0)==1);
 assert(etch_pre_mix(&s,0,0)>15000);
 assert(footer(&s,&e,0)!=old);
 /* Independent A/B slots, one detent always changes depth and mix. */
 uint16_t first=s.pre[0][0];etch_v117_pre_turn(&s,0,0,0,1,-1);
 assert(s.pre[0][0]!=first && etch_pre_type(&s,0,0)==1);
 assert(etch_pre_type(&s,1,0)==0);
 first=s.pre[0][0];etch_v117_pre_turn(&s,1,0,3,2,1);
 assert(s.pre[0][0]==first);
 assert(etch_pre_type(&s,1,0)==3 && etch_pre_mix(&s,1,0)>15000);
 assert(etch_v117_pre_slot(0,1)==1 && etch_v117_pre_slot(1,5)==1);
 assert(etch_v117_pre_slot(0,-4)==0);
 uint32_t h=footer(&s,&e,0);
 assert(footer(&s,&e,1)!=h);
 /* Every Evolution knob independently changes its own parameter + LCD key. */
 etch_v117_evolution_turn(&e,1,0,0,0,0);
 assert(e.depth==5 && e.rate==30 && e.mode==0 && e.sync==0);
 assert(footer(&s,&e,0)!=h);h=footer(&s,&e,0);
 etch_v117_evolution_turn(&e,0,1,0,0,0);
 assert(e.depth==5 && e.rate==35 && e.mode==0 && e.sync==0);
 assert(footer(&s,&e,0)!=h);h=footer(&s,&e,0);
 etch_v117_evolution_turn(&e,0,0,1,0,0);
 assert(e.mode==1 && e.sync==0 && footer(&s,&e,0)!=h);h=footer(&s,&e,0);
 etch_v117_evolution_turn(&e,0,0,0,1,0);
 assert(e.sync==1 && footer(&s,&e,0)!=h);
 etch_v117_evolution_turn(&e,0,0,0,0,1);
 assert(e.depth==0);
 etch_v117_evolution_turn(&e,0,0,0,0,1);
 assert(e.depth==60);
 e.depth=100;e.rate=100;e.mode=2;e.sync=2;
 etch_v117_evolution_turn(&e,100,100,100,100,0);
 assert(e.depth==100 && e.rate==100 && e.mode==2 && e.sync==2);
 puts("PASS V1.17 A/B FX four knobs + audible default, Evolution four knobs and footer invalidation");
 return 0;
}
