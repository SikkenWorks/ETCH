/* V1.10: repeatable dry FX, original preset data, tracked UI trigger. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_fx.h"
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_ui_wave.h"
static struct etch_native_fx fx;
static struct etch_project_settings cfg,outcfg;
static struct etch_wave_library lib,outlib;
static struct etch_native_pattern pattern,outpattern;
static uint8_t bytes[ETCH_PROJECT_MAX_BYTES];
int main(void){
 struct etch_fx_settings s;etch_fx_defaults(&s);
 assert(etch_fx_controls_dry(&s,0,0,0));
 assert(!etch_fx_controls_dry(&s,1,0,0));
 assert(!etch_fx_controls_dry(&s,0,0,2));
 etch_fx_reset(&fx);
 for(int i=0;i<32000;i++){
  int16_t x=(int16_t)(((i*1381)%55001)-27500),l=0,r=0;
  etch_fx_process_stereo_ex(&fx,x,0,0,0,&s,&l,&r);
  assert(l==x&&r==x); /* Exact, not merely similar dry audio */
 }
 /* Rates, depths and feedback may be saved but must not activate an effect. */
 s.tape_wow=32767;s.tape_flutter=32767;s.phaser_feedback=32767;
 s.reverse_feed=32767;s.decim_drive=32767;s.folder_fold=32767;
 assert(etch_fx_controls_dry(&s,0,0,0));
 s.tape_mix=16384;assert(!etch_fx_controls_dry(&s,0,0,0));
 s.tape_mix=0;s.phaser_mix=20000;assert(!etch_fx_controls_dry(&s,0,0,0));
 s.phaser_mix=0;s.eq_mid=19000;assert(!etch_fx_controls_dry(&s,0,0,0));
 s.eq_mid=16384;assert(etch_fx_controls_dry(&s,0,0,0));
 /* An intended wet preset must remain wet, a dry preset must stay dry. */
 memset(&cfg,0,sizeof(cfg));cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;
 cfg.source_version=17;etch_fx_defaults(&cfg.global_fx);
 cfg.preset_valid=3;lib.patches[0].valid=lib.patches[1].valid=1;
 etch_fx_defaults(&cfg.preset_fx[0]);etch_fx_defaults(&cfg.preset_fx[1]);
 cfg.preset_fx[1].tape_mix=18000;cfg.preset_fx[1].phaser_mix=20000;
 cfg.preset_fx[1].eq_high=20000;
 cfg.preset_fx[0].pre[0][0]=0;cfg.preset_fx[1].pre[0][0]=5;
 uint32_t used=0;
 assert(etch_project_encode(&lib,&pattern,&cfg,bytes,sizeof(bytes),&used)==0);
 assert(etch_project_decode(bytes,used,&outlib,&outpattern,&outcfg)==0);
 assert(outcfg.source_version==17);
 assert(!memcmp(&cfg.preset_fx[0],&outcfg.preset_fx[0],sizeof(struct etch_fx_settings)));
 assert(!memcmp(&cfg.preset_fx[1],&outcfg.preset_fx[1],sizeof(struct etch_fx_settings)));
 assert(etch_fx_controls_dry(&outcfg.preset_fx[0],0,0,0));
 assert(!etch_fx_controls_dry(&outcfg.preset_fx[1],0,0,0));
 /* 1024 captured samples, 512 visible; stabilize the trigger at the nearest
  * rising zero-cross instead of jumping to the first crossing on each frame. */
 int16_t samples[1024];
 for(int i=0;i<1024;i++)samples[i]=(int16_t)((i%64<32)?-8000:8000);
 uint16_t near=etch_ui_scope_trigger_near(samples,1024,512,365);
 assert(near==352u || near==416u);
 assert(etch_ui_scope_trigger_near(samples,1024,512,near)==near);
 int16_t silence[1024]={0};assert(etch_ui_scope_trigger_near(silence,1024,512,near)==near);
 assert(etch_ui_live_frame_interval(8,0,0)==50u);
 assert(etch_ui_live_frame_interval(8,1,0)==80u);
 assert(etch_ui_live_frame_interval(8,3,0)==120u);
 puts("PASS V1.10 professional neutral FX, PJT17 preset independence, steady scope trigger");
}
