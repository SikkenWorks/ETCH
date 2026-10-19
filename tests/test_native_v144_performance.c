/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_home_shortcuts.h"
static struct etch_wave_library lib,decoded;
static struct etch_native_pattern pat,decoded_pat;
static struct etch_project_settings cfg,output;
static struct etch_preset_dsp_bank dsp,decoded_dsp;
static uint8_t buffer[ETCH_PROJECT_MAX_BYTES];
int main(void){
 etch_project_legacy(&lib,&pat,&cfg);
 cfg.source_version=31;cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;
 etch_project_bind_preset_dsp(&cfg,&dsp);
 etch_project_bind_preset_dsp(&output,&decoded_dsp);
 etch_key_groups_init(&dsp.groups);
 etch_perf_init(&dsp.performance);
 assert(etch_perf_valid(&dsp.performance));
 assert(etch_perf_q15(0)==0&&etch_perf_q15(100)==32767);
 assert(etch_perf_percent(32767)==100&&etch_perf_percent(0)==0);
 assert(etch_perf_cutoff(0)==20&&etch_perf_cutoff(100)==18000);
 assert(etch_perf_cutoff_pct(20)==0&&etch_perf_cutoff_pct(18000)==100);
 assert(etch_perf_change(0,-5)==0&&etch_perf_change(100,5)==100);
 dsp.performance.target[0]=4;
 uint8_t a[4]={5,27,56,99},b[4]={88,77,22,0};
 etch_perf_capture(&dsp.performance,0,a);etch_perf_capture(&dsp.performance,1,b);
 assert(etch_perf_valid(&dsp.performance));
 uint8_t out[4]={0};assert(etch_perf_recall(&dsp.performance,1,out));
 assert(!memcmp(out,b,4));
 uint32_t size=0;
 assert(etch_project_encode(&lib,&pat,&cfg,buffer,sizeof(buffer),&size)==0);
 assert(!memcmp(buffer,"PJTO",4));
 assert(etch_project_decode(buffer,size,&decoded,&decoded_pat,&output)==0);
 assert(output.source_version==31);
 assert(!memcmp(&decoded_dsp.performance,&dsp.performance,sizeof(dsp.performance)));
 assert(etch_project_decode(buffer,size-1,&decoded,&decoded_pat,&output)!=0);
 cfg.source_version=30;
 assert(etch_project_encode(&lib,&pat,&cfg,buffer,sizeof(buffer),&size)==0);
 assert(!memcmp(buffer,"PJTN",4));
 assert(etch_project_decode(buffer,size,&decoded,&decoded_pat,&output)==0);
 assert(output.source_version==30);
 assert(decoded_dsp.performance.valid==0&&decoded_dsp.performance.target[0]==0);
 assert(decoded_dsp.performance.target[1]==1&&decoded_dsp.performance.target[2]==2);
 assert(decoded_dsp.performance.target[3]==3);
 /* Old-format byte layout unchanged and existing projects do not acquire a hidden extension. */
 struct etch_home_shortcuts hs={0};
 uint32_t home=1u<<8,arp=1u<<10,fx=1u<<2,save=1u<<9;
 assert(etch_home_shortcuts_scan(&hs,home|arp,home|arp)==ETCH_SHORTCUT_PERFORMANCE);
 assert(etch_home_shortcuts_scan(&hs,home|arp,0)==ETCH_SHORTCUT_NONE);
 assert(etch_home_shortcuts_scan(&hs,0,0)==ETCH_SHORTCUT_NONE);
 assert(etch_home_shortcuts_scan(&hs,home|fx,home|fx)==ETCH_SHORTCUT_GROUPS);
 assert(etch_home_shortcuts_scan(&hs,0,0)==ETCH_SHORTCUT_NONE);
 assert(etch_home_shortcuts_scan(&hs,home|save,home|save)==ETCH_SHORTCUT_PROJECTS);
 puts("PASS V1.44 PJT30/31 roundtrip, 2 scenes, 4 macros, bounded values, home shortcuts");
}
