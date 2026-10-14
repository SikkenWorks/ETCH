/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/audio_core.h"
static struct etch_native_audio a;
static struct etch_native_controls c;
static int16_t audio[128];
static void run_second(void){for(int k=0;k<344;k++)etch_native_render(&a,&c,1u,audio,128);}
static uint32_t riff_count(uint8_t bpm,uint8_t rate){
 etch_native_reset(&a);memset(&c,0,sizeof(c));
 c.arp_mode=7;c.arp_rate=rate;c.arp_octaves=1;c.tempo=bpm;
 c.gain=32767;c.riff_seed=0x5928;
 run_second();return a.perf_index;
}
int main(void){
 assert(riff_count(60,2)==4);
 assert(riff_count(120,2)==8);
 assert(riff_count(180,2)==12);
 assert(riff_count(120,1)==4);
 assert(riff_count(120,3)==16);
 etch_native_reset(&a);memset(&c,0,sizeof(c));
 c.arp_mode=7;c.arp_rate=2;c.arp_octaves=1;c.tempo=60;
 run_second();uint32_t before=a.perf_index;assert(before==4);
 c.tempo=120;run_second();assert(a.perf_index>=before+7 && a.perf_index<=before+9);
 etch_native_reset(&a);memset(&c,0,sizeof(c));
 c.arp_mode=7;c.arp_rate=2;c.arp_octaves=1;c.tempo=60;c.clock_follow=1;
 a.clock_bpm=180;a.clock_running=1;
 run_second();assert(a.perf_index==12);
 puts("PASS V1.24 RIFF BPM 60/120/180; RATE divides beats; live tempo; MIDI clock follow");
}
