/* ETCH V1.11: deterministic quiet-output and tempo regression. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_tempo_restore.h"
#include "../firmware/fm1-native/native_fx.h"
static struct etch_native_audio engine;
static struct etch_native_fx effects;
static struct etch_native_controls ctrl;
static int16_t out[128];
int main(void){
 struct etch_fx_settings fx;etch_fx_defaults(&fx);
 /* Legacy preset may carry an unrelated BPM; project owns transport tempo. */
 assert(etch_saved_project_bpm(137,120)==137);
 assert(etch_saved_project_bpm(0,154)==154);
 assert(etch_saved_project_bpm(0,0)==120);
 /* Wavefolder with asymmetric fold must never self-oscillate or generate DC. */
 fx.folder_fold=32767;fx.folder_symmetry=29000;fx.folder_mix=32767;
 etch_fx_reset(&effects);
 for(int n=0;n<4096;n++){
  int16_t l=123,r=123;
  etch_fx_process_stereo_ex(&effects,0,0,0,0,&fx,&l,&r);
  if(n>1000){assert(l==0);assert(r==0);}
 }
 etch_native_reset(&engine);
 memset(&ctrl,0,sizeof(ctrl));ctrl.gain=32767;ctrl.tempo=120;ctrl.fx_settings=fx;
 /* During a cold boot or idle, the synth output must stay digitally silent. */
 for(int k=0;k<400;k++){
  etch_native_render(&engine,&ctrl,0,out,128);
  for(int j=0;j<128;j++) assert(out[j]==0);
 }
 /* A real note must reopen the gate and produce an audible oscillator. */
 etch_native_render(&engine,&ctrl,1,out,128);
 int found=0;
 for(int k=0;k<24;k++){
  etch_native_render(&engine,&ctrl,1,out,128);
  for(int j=0;j<128;j++)if(out[j]!=0)found=1;
 }
 assert(found);
 /* A note-off must decay, then silence gate must suppress perpetual FX DC. */
 for(int k=0;k<1400;k++)etch_native_render(&engine,&ctrl,0,out,128);
 for(int j=0;j<128;j++)assert(out[j]==0);
 puts("PASS V1.11 silent voice path, asymmetric wavefolder DC removal, note recovery and project BPM precedence");
 return 0;
}
