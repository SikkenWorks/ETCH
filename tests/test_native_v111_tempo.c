#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_preset_defaults.h"
static struct etch_native_audio a;
static struct etch_native_controls c;
static int16_t buffer[128];
static uint32_t played_steps(uint8_t bpm){
 etch_native_reset(&a);memset(&c,0,sizeof(c));
 c.seq_run=1;c.tempo=bpm;c.seq_rate=2;c.seq_length=16;
 for(int i=0;i<64;i++)c.seq_notes[i]=255;
 for(int k=0;k<344;k++)etch_native_render(&a,&c,0,buffer,128);
 /* 344*128 = 44032 samples, within one audio block of one second. */
 return a.perf_index; /* seq uses perf_step instead; evaluate as modulo-16. */
}
int main(void){
 struct etch_fx_settings fx;struct etch_wave_fx_params wave={0};
 etch_fx_defaults(&fx);fx.c_fx=0xffff;fx.phaser_mix=1234;
 wave.drive=8000;wave.mix=32767;
 etch_preset_legacy_c_fx_off(&wave,&fx);
 assert(fx.c_fx==0 && fx.phaser_mix==1234);
 assert(wave.drive==0 && wave.fold==0 && wave.mix==32767);
 /* Rate=2 means four notes/beat; one second gives BPM/15 steps. */
 (void)played_steps(60);assert(a.perf_step==4);
 (void)played_steps(120);assert(a.perf_step==8);
 (void)played_steps(180);assert(a.perf_step==12);
 puts("PASS V1.11 BPM playback rate 60/120/180, legacy Wave C FX defaults, preserved intentional master FX");
}
