/* ETCH V1.04: tape DSP / PJT15 roundtrip / backward compatibility. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../firmware/fm1-native/native_fx.h"
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_ui_wave.h"
#include "../firmware/fm1-native/native_ui_navigation.h"
static struct etch_native_fx fx;
static struct etch_wave_library l,l2;
static struct etch_native_pattern p,p2;
static struct etch_project_settings cfg,got;
static uint8_t encoded[ETCH_PROJECT_MAX_BYTES];
int main(void){
 struct etch_fx_settings c;etch_fx_defaults(&c);etch_fx_reset(&fx);
 /* Dry Tape: exact samples; en/disabling other FX must not change legacy neutral. */
 for(int i=0;i<6000;i++){
  int16_t x=(int16_t)(((i*163)%32767)-16384);
  assert(etch_fx_process_ex(&fx,x,0,0,0,&c)==x);
 }
 etch_fx_defaults(&c);c.tape_wow=32767;c.tape_flutter=32767;c.tape_age=18000;c.tape_mix=32767;
 etch_fx_reset(&fx);unsigned modified=0;int16_t last=0;
 for(int i=0;i<15000;i++){
  int16_t x=(int16_t)((i%77<38)?11000:-11000);
  int16_t y=etch_fx_process_ex(&fx,x,0,0,0,&c);
  if(y!=x)modified++;
  if(i>1000)assert(abs((int)y-(int)last)<=22000);
  last=y;
 }
 assert(modified>10000);
 /* PLAY: visual-only midpoint; asymmetric waveform must stay symmetrically centered. */
 int16_t shifted[128];for(int i=0;i<128;i++)shifted[i]=(int16_t)(i%2?12000:4000);
 int32_t center=etch_ui_scope_midpoint(shifted,128);
 assert(center==8000);
 assert(etch_ui_scope_centered(shifted[0],center)==-4000);
 assert(etch_ui_scope_centered(shifted[1],center)==4000);
 uint16_t g=etch_ui_scope_centered_gain(shifted,128,center);
 uint32_t high=etch_ui_wave_display_y(etch_ui_scope_centered(shifted[1],center),g,98,51,43,153);
 uint32_t low=etch_ui_wave_display_y(etch_ui_scope_centered(shifted[0],center),g,98,51,43,153);
 assert((int)(high+low)>=(2*98-1) && (int)(high+low)<=(2*98+1));
 int16_t silence[32]={0};assert(etch_ui_scope_midpoint(silence,32)==0);
 assert(etch_ui_scope_centered(-32768,32767)==-32768);
 /* Extra page reachable in both directions; legacy FX pages preserved. */
 assert(etch_ui_fx_select(ETCH_UI_FX_FOLDER,1)==ETCH_UI_FX_TAPE);
 assert(etch_ui_fx_select(ETCH_UI_FX_TAPE,1)==ETCH_UI_FX_ROTARY);
 assert(etch_ui_fx_select(ETCH_UI_FX_DRIVE,-1)==ETCH_UI_FX_WAH);
 assert(strcmp(etch_ui_title(ETCH_UI_FX_TAPE),"TAPE INSTABILITY")==0);
 assert(88+strlen("TAPE INSTAB.")*8<196); /* header version can never overlap */
 memset(&cfg,0,sizeof(cfg));etch_pattern_clear(&p);
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=138;
 cfg.source_version=15;etch_fx_defaults(&cfg.global_fx);
 cfg.global_fx.tape_wow=27300;cfg.global_fx.tape_flutter=13100;
 cfg.global_fx.tape_age=20600;cfg.global_fx.tape_mix=29100;
 l.patches[0].valid=1;l.patches[1].valid=1;cfg.preset_valid=3;
 etch_fx_defaults(&cfg.preset_fx[0]);etch_fx_defaults(&cfg.preset_fx[1]);
 cfg.preset_fx[0].tape_mix=31000;cfg.preset_fx[0].tape_age=20000;
 cfg.preset_fx[1].tape_mix=7000;cfg.preset_fx[1].tape_flutter=26000;
 for(int j=0;j<3;j++)for(int k=0;k<128;k++){
  cfg.preset_wave[0][j][k]=(int16_t)(k*13-3000);
  cfg.preset_wave[1][j][k]=(int16_t)(k*11+400);
 }
 uint32_t bytes=0;
 assert(etch_project_encode(&l,&p,&cfg,encoded,sizeof(encoded),&bytes)==0);
 assert(bytes>100 && !memcmp(encoded,"PJT?",4));
 assert(etch_project_decode(encoded,bytes,&l2,&p2,&got)==0);
 assert(got.source_version==15&&got.tempo==138);
 assert(got.global_fx.tape_wow==27300&&got.global_fx.tape_mix==29100);
 assert(got.preset_fx[0].tape_mix==31000&&got.preset_fx[1].tape_flutter==26000);
 assert(memcmp(got.preset_wave[1],cfg.preset_wave[1],sizeof(cfg.preset_wave[1]))==0);
 assert(etch_project_decode(encoded,bytes-1,&l2,&p2,&got)!=0);
 encoded[bytes-1]=0xff;encoded[bytes-2]=0xff;
 assert(etch_project_decode(encoded,bytes,&l2,&p2,&got)!=0); /* above-Q15 corruption */
 cfg.source_version=14;
 assert(etch_project_encode(&l,&p,&cfg,encoded,sizeof(encoded),&bytes)==0);
 assert(!memcmp(encoded,"PJT>",4));
 assert(etch_project_decode(encoded,bytes,&l2,&p2,&got)==0);
 assert(got.source_version==14&&got.global_fx.tape_mix==0);
 assert(got.preset_fx[0].tape_mix==0);
 puts("PASS V1.04: exact dry, audible tape, safe scope centering, FX UI page, PJT15/PJT14 compatibility");
 return 0;
}
