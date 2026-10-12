/* V1.06 four new FX: fixed CPU-safe DSP / navigation / PJT16 migration */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../firmware/fm1-native/native_fx.h"
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_ui_navigation.h"
static struct etch_native_fx fx;
static struct etch_wave_library library,outlib;
static struct etch_native_pattern pat,outpat;
static struct etch_project_settings cfg,outcfg;
static uint8_t bytes[ETCH_PROJECT_MAX_BYTES];
static int16_t input(int n){return (int16_t)(((n*1103)%32767)-16383);}
static int audible(struct etch_fx_settings *s){
 etch_fx_reset(&fx);unsigned diff=0;
 for(int n=0;n<25000;n++){int16_t x=input(n),y=etch_fx_process_ex(&fx,x,0,0,0,s);if(x!=y)diff++;}
 return diff>2000;
}
int main(void){
 struct etch_fx_settings p;etch_fx_defaults(&p);etch_fx_reset(&fx);
 for(int n=0;n<12000;n++){int16_t x=input(n);assert(etch_fx_process_ex(&fx,x,0,0,0,&p)==x);}
 etch_fx_defaults(&p);p.vibrato_mix=30000;p.vibrato_depth=32000;assert(audible(&p));
 etch_fx_defaults(&p);p.wah_mix=31000;p.wah_sens=32767;assert(audible(&p));
 etch_fx_defaults(&p);p.reverse_mix=32767;p.reverse_time=32000;assert(audible(&p));
 etch_fx_defaults(&p);p.rotary_mix=32767;p.rotary_depth=32767;
 etch_fx_reset(&fx);unsigned width=0;for(int n=0;n<25000;n++){int16_t x=input(n),l,r;etch_fx_process_stereo_ex(&fx,x,0,0,0,&p,&l,&r);if(l!=r)width++;}assert(width>2000);
 etch_fx_defaults(&p);p.vibrato_mix=32767;p.wah_mix=32767;p.reverse_mix=32767;p.rotary_mix=32767;
 etch_fx_reset(&fx);for(int n=0;n<30000;n++){int16_t x=input(n);int16_t l,r;etch_fx_process_stereo_ex(&fx,x,0,0,0,&p,&l,&r);(void)l;(void)r;}
 assert(etch_ui_fx_select(ETCH_UI_FX_DELAY,1)==ETCH_UI_FX_REVERSE);
 assert(etch_ui_fx_select(ETCH_UI_FX_REVERSE,1)==ETCH_UI_FX_REVERB);
 assert(etch_ui_fx_select(ETCH_UI_FX_TREM,1)==ETCH_UI_FX_VIBRATO);
 assert(etch_ui_fx_select(ETCH_UI_FX_VIBRATO,1)==ETCH_UI_FX_PHASER);
 assert(etch_ui_fx_select(ETCH_UI_FX_DRIVE,-1)==ETCH_UI_FX_WAH);
 for(int j=0;j<17;j++)assert(etch_ui_fx_select(ETCH_UI_FX_DRIVE,j)==etch_ui_fx_select(ETCH_UI_FX_DRIVE,j+17));
 memset(&cfg,0,sizeof(cfg));cfg.source_version=16;cfg.tempo=123;cfg.seq_length=16;cfg.seq_gate=75;
 etch_fx_defaults(&cfg.global_fx);
 cfg.global_fx.reverse_mix=32767;cfg.global_fx.reverse_feed=17321;
 cfg.global_fx.wah_mix=16000;cfg.global_fx.rotary_rate=21345;
 cfg.preset_valid=3u;library.patches[0].valid=1;library.patches[1].valid=1;
 etch_fx_defaults(&cfg.preset_fx[0]);etch_fx_defaults(&cfg.preset_fx[1]);
 cfg.preset_fx[0].vibrato_mix=30000;cfg.preset_fx[0].vibrato_depth=12345;
 cfg.preset_fx[1].reverse_mix=10000;cfg.preset_fx[1].rotary_mix=29999;
 for(int i=0;i<128;i++){cfg.preset_wave[0][0][i]=(int16_t)(i*10-1200);cfg.preset_wave[1][0][i]=(int16_t)(i*11-1320);}
 uint32_t len=0;
 assert(etch_project_encode(&library,&pat,&cfg,bytes,sizeof(bytes),&len)==0);
 assert(!memcmp(bytes,"PJT@",4));
 assert(etch_project_decode(bytes,len,&outlib,&outpat,&outcfg)==0);
 assert(outcfg.source_version==16&&outcfg.tempo==123);
 assert(outcfg.global_fx.reverse_feed==17321&&outcfg.global_fx.wah_mix==16000);
 assert(outcfg.preset_fx[0].vibrato_depth==12345&&outcfg.preset_fx[1].rotary_mix==29999);
 assert(etch_project_decode(bytes,len-1,&outlib,&outpat,&outcfg)!=0);
 cfg.source_version=15;assert(etch_project_encode(&library,&pat,&cfg,bytes,sizeof(bytes),&len)==0);
 assert(etch_project_decode(bytes,len,&outlib,&outpat,&outcfg)==0);
 assert(outcfg.source_version==15&&outcfg.global_fx.reverse_mix==0);
 assert(outcfg.preset_fx[0].vibrato_mix==0);
 puts("PASS V1.06: new DSP, dry, navigation, PJT16/PJT15 codec");
}
