/* GPL-3.0-only: ETCH V1.07 pre-morph A/B FX and Wave C master modulation */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_encoder_steps.h"
#include "../firmware/fm1-native/native_ui_navigation.h"
static struct etch_native_audio audio;
static int16_t pcm[128];
static struct etch_project_settings settings,read_settings;
static struct etch_wave_library library,read_library;
static struct etch_native_pattern pattern,read_pattern;
static uint8_t bytes[ETCH_PROJECT_MAX_BYTES];
static struct etch_native_controls ctrl;
static long energy(void){
 etch_native_reset(&audio);audio.morph=ctrl.morph;
 ctrl.depth=0;ctrl.gain=32767;
 ctrl.sustain_level=32767;ctrl.env_enabled=0;
 long total=0;
 for(int n=0;n<26;n++){
  etch_native_render(&audio,&ctrl,1u,pcm,128);
  if(n>=4)for(int i=0;i<128;i++)total+=labs((long)pcm[i]);
 }
 return total;
}
static unsigned change_from_baseline(const int16_t *baseline){
 etch_native_reset(&audio);unsigned changed=0;
 for(int n=0;n<18;n++){
  etch_native_render(&audio,&ctrl,1u,pcm,128);
  if(n>=4)for(int i=0;i<128;i++)if(pcm[i]!=baseline[(n-4)*128+i])++changed;
 }
 return changed;
}
int main(void){
 assert(etch_encoder_cutoff(7000,1)==7020);
 assert(etch_encoder_lock_value(300,2,1)==320);
 assert(etch_encoder_lock_value(300,0,1)==301);
 assert(sizeof(struct etch_fx_settings)==128u);
 assert(etch_ui_title(ETCH_UI_PREFX_A)[5]=='A');
 assert(etch_ui_title(ETCH_UI_PREFX_B)[5]=='B');
 assert(etch_ui_title(ETCH_UI_C_FX)[5]=='C');
 etch_fx_defaults(&ctrl.fx_settings);
 long dry=energy();assert(dry>10000);
 etch_pre_set_type(&ctrl.fx_settings,0,0,3); /* FOLD pre morph */
 etch_pre_set_depth(&ctrl.fx_settings,0,0,29000);
 etch_pre_set_mix(&ctrl.fx_settings,0,0,32767);
 long folded=energy();assert(folded!=dry);
 /* A-only pre FX must have no audible impact at 100% B morph */
 ctrl.morph=32767;long b_energy=energy();
 etch_pre_set_mix(&ctrl.fx_settings,0,0,0);
 long b_without=energy();fprintf(stderr,"B energy %ld vs %ld\n",b_energy,b_without); assert(labs(b_without-b_energy)<1000);
 ctrl.morph=0;
 etch_pre_set_type(&ctrl.fx_settings,0,0,1);
 etch_pre_set_mix(&ctrl.fx_settings,0,0,25000);
 etch_pre_set_type(&ctrl.fx_settings,1,0,2);
 etch_pre_set_depth(&ctrl.fx_settings,1,0,12000);
 etch_pre_set_mix(&ctrl.fx_settings,1,0,28000);
 ctrl.morph=16384;
 assert(energy()>100);
 /* Exact bypass: every insert MIX=0. Wave C modulation OFF. */
 etch_pre_set_mix(&ctrl.fx_settings,0,0,0);
 etch_pre_set_mix(&ctrl.fx_settings,1,0,0);
 etch_c_fx_set_target(&ctrl.fx_settings,0);
 assert(etch_c_fx_target(&ctrl.fx_settings)==0);
 ctrl.fx_settings.trem_mix=30000;ctrl.fx_settings.trem_depth=18000;
 etch_native_reset(&audio);
 int16_t dry_wave[14*128];
 for(int n=0;n<18;n++){
  etch_native_render(&audio,&ctrl,1u,pcm,128);
  if(n>=4)memcpy(&dry_wave[(n-4)*128],pcm,sizeof(pcm));
 }
 etch_c_fx_set_target(&ctrl.fx_settings,5); /* Wave C => trem depth */
 etch_c_fx_set_depth(&ctrl.fx_settings,31000);
 assert(change_from_baseline(dry_wave)>50);
 /* PJT17 project/preset saves must retain independent effect routings. */
 memset(&settings,0,sizeof(settings));settings.seq_length=16;
 settings.seq_gate=75;settings.tempo=120;settings.source_version=17;
 etch_fx_defaults(&settings.global_fx);
 settings.preset_valid=3;library.patches[0].valid=1;library.patches[1].valid=1;
 etch_fx_defaults(&settings.preset_fx[0]);etch_fx_defaults(&settings.preset_fx[1]);
 etch_pre_set_type(&settings.global_fx,0,0,2);
 etch_pre_set_mix(&settings.global_fx,0,0,17000);
 etch_c_fx_set_target(&settings.global_fx,3);
 etch_c_fx_set_depth(&settings.global_fx,12400);
 etch_pre_set_type(&settings.preset_fx[0],0,1,3);
 etch_pre_set_depth(&settings.preset_fx[0],0,1,32000);
 etch_pre_set_mix(&settings.preset_fx[0],0,1,27000);
 etch_pre_set_type(&settings.preset_fx[1],1,1,5);
 etch_pre_set_mix(&settings.preset_fx[1],1,1,16000);
 uint32_t used=0;
 assert(etch_project_encode(&library,&pattern,&settings,bytes,sizeof(bytes),&used)==0);
 assert(!memcmp(bytes,"PJTA",4));
 assert(etch_project_decode(bytes,used,&read_library,&read_pattern,&read_settings)==0);
 assert(read_settings.source_version==17);
 assert(etch_c_fx_target(&read_settings.global_fx)==etch_c_fx_target(&settings.global_fx));
 assert(labs((long)etch_c_fx_depth(&read_settings.global_fx)-(long)etch_c_fx_depth(&settings.global_fx))<50); /* legacy 13->11 bit migration */
 assert(read_settings.global_fx.pre[0][0]==settings.global_fx.pre[0][0]);
 assert(read_settings.preset_fx[0].pre[0][1]==settings.preset_fx[0].pre[0][1]);
 assert(read_settings.preset_fx[1].pre[1][1]==settings.preset_fx[1].pre[1][1]);
 assert(etch_project_decode(bytes,used-1,&read_library,&read_pattern,&read_settings)!=0);
 /* PJT16 remains readable with all new FX truly disabled. */
 settings.source_version=16;
 assert(etch_project_encode(&library,&pattern,&settings,bytes,sizeof(bytes),&used)==0);
 assert(etch_project_decode(bytes,used,&read_library,&read_pattern,&read_settings)==0);
 assert(read_settings.source_version==16);
 assert(etch_pre_type(&read_settings.global_fx,0,0)==0);
 assert(etch_c_fx_target(&read_settings.global_fx)==0);
 puts("PASS V1.07 CUTOFF +20 / pre-morph A/B inserts / C -> FX / PJT16->17 compat");
}
