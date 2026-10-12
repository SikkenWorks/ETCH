/* V1.12: dry FX cleanup and stable/cheap LCD scope, host only. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_fx.h"
#include "../firmware/fm1-native/native_ui_wave.h"
static struct etch_native_fx fx;
int main(void){
 struct etch_fx_settings cfg;etch_fx_defaults(&cfg);
 etch_fx_reset(&fx);
 int16_t l=0,r=0;
 /* Starting with an untouched dry preset is EXACT dry and does not scrub
    memory needlessly. Rate and feedback may retain saved knob positions. */
 for(int i=0;i<100;i++){
  int16_t x=(int16_t)((i*101)%20000-10000);
  etch_fx_process_stereo_ex(&fx,x,0,0,0,&cfg,&l,&r);
  assert(l==x&&r==x);
 }
 assert(!fx.dry_flush_active && !fx.dry_was_wet);
 /* A wet preset populates long delay/reverb buffers. */
 for(int i=0;i<12000;i++){
  int16_t x=(int16_t)((i%64<32)?12000:-12000);
  etch_fx_process_stereo_ex(&fx,x,13000,11000,12000,&cfg,&l,&r);
 }
 assert(fx.dry_was_wet);
 /* Stored settings remain untouched; dry bypass is exact during scrub. */
 struct etch_fx_settings saved=cfg;
 for(int i=0;i<ETCH_DELAY_SIZE+100;i++){
  int16_t x=(int16_t)((i%89)-44);
  etch_fx_process_stereo_ex(&fx,x,0,0,0,&cfg,&l,&r);
  assert(l==x&&r==x);
 }
 assert(!memcmp(&saved,&cfg,sizeof(cfg)));
 assert(!fx.dry_flush_active && !fx.dry_was_wet);
 for(unsigned i=0;i<ETCH_DELAY_SIZE;i++)assert(fx.delay[i]==0);
 for(unsigned i=0;i<ETCH_REVERB_SIZE;i++)assert(fx.reverb[i]==0);
 for(unsigned i=0;i<ETCH_CHORUS_SIZE;i++)assert(fx.chorus[i]==0);
 /* Re-enabling a time FX with no note cannot resurrect an earlier patch. */
 for(int i=0;i<900;i++){
  etch_fx_process_stereo_ex(&fx,0,0,16000,10000,&cfg,&l,&r);
  assert(l==0&&r==0);
 }
 /* One-pass measurements match old frame functions, including negative peak. */
 int16_t frame[512];for(int i=0;i<512;i++)frame[i]=(int16_t)((i*257)%39001-19000);
 frame[17]=-32768;
 int32_t midpoint=0;uint32_t peak=0;
 etch_ui_scope_stats(frame,512,&midpoint,&peak);
 assert(midpoint==etch_ui_scope_midpoint(frame,512));
 assert(peak==32768u);
 int32_t prior=0;for(int i=0;i<32;i++)prior=etch_ui_scope_smooth_center(prior,4000,0);
 assert(prior>3900&&prior<=4000);
 for(int i=0;i<32;i++)prior=etch_ui_scope_smooth_center(prior,-3000,0);
 assert(prior>=-3000&&prior<-2900);
 assert(etch_ui_scope_smooth_center(123,-800,1)==-800);
 etch_ui_scope_stats(NULL,0,&midpoint,&peak);assert(midpoint==0&&peak==0);
 puts("PASS V1.12 exact dry sound, scrubbed FX histories, no stale tails, single-pass stable scope");
}
