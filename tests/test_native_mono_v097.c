/* SPDX-License-Identifier: GPL-3.0-only
 * V0.97 chord, mono allocator, PJT11 -> PJT12 compatibility.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_octave_mode.h"
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_wave_library src,dst;
static struct etch_native_pattern pat,pat2;
static struct etch_project_settings cfg,got;
static uint8_t bytes[ETCH_PROJECT_MAX_BYTES];
int main(void){
 struct etch_octave_chord chord={0};
 assert(etch_octave_scan(&chord,0)==ETCH_OCT_NONE);
 assert(etch_octave_scan(&chord,1)==ETCH_OCT_NONE);
 assert(etch_octave_scan(&chord,3)==ETCH_OCT_TOGGLE);
 assert(etch_octave_scan(&chord,3)==ETCH_OCT_NONE);
 assert(etch_octave_scan(&chord,1)==ETCH_OCT_NONE);
 assert(etch_octave_scan(&chord,0)==ETCH_OCT_NONE);
 assert(etch_octave_scan(&chord,2)==ETCH_OCT_NONE);
 assert(etch_octave_scan(&chord,0)==ETCH_OCT_PLUS);
 assert(etch_octave_scan(&chord,1)==ETCH_OCT_NONE);
 assert(etch_octave_scan(&chord,0)==ETCH_OCT_MINUS);
 assert(etch_octave_scan(&chord,3)==ETCH_OCT_TOGGLE);
 assert(etch_octave_scan(&chord,0)==ETCH_OCT_NONE);
 assert(etch_octave_scan(&chord,3)==ETCH_OCT_TOGGLE);
 assert(etch_octave_scan(&chord,0)==ETCH_OCT_NONE);
 struct etch_native_audio audio;
 struct etch_native_controls ctl={0};int16_t frame[2*128];
 etch_native_reset(&audio);ctl.mono_mode=1;ctl.gain=32767;ctl.sustain_level=32767;
 ctl.attack_step=32767;ctl.decay_step=32767;ctl.release_step=32767;
 ctl.filter_type=0;ctl.env_enabled=1;
 etch_native_render_stereo(&audio,&ctl,3u,frame,128);
 unsigned active=0;for(int i=0;i<8;i++)active+=(unsigned)(audio.voice[i].active!=0);
 assert(audio.mono_mode==1&&active==1&&audio.voice_limit==8);
 ctl.mono_mode=0;
 etch_native_render_stereo(&audio,&ctl,0,frame,128);
 etch_native_render_stereo(&audio,&ctl,3u,frame,128);
 active=0;for(int i=0;i<8;i++)active+=(unsigned)(audio.voice[i].active!=0);
 assert(audio.mono_mode==0&&active>=2);
 etch_pattern_clear(&pat);cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=137;
 etch_fx_defaults(&cfg.global_fx);
 cfg.mono_mode=1;
 uint32_t used=0;
 assert(etch_project_encode(&src,&pat,&cfg,bytes,sizeof(bytes),&used)==0);
 assert(!memcmp(bytes,"PJT<",4));
 assert(etch_project_decode(bytes,used,&dst,&pat2,&got)==0);
 assert(got.source_version==12&&got.mono_mode==1&&got.tempo==137);
 /* Older PJT11 payload is exactly the same except for the final mode byte. */
 bytes[3]=';';
 assert(etch_project_decode(bytes,used-1,&dst,&pat2,&got)==0);
 assert(got.source_version==11&&got.mono_mode==0&&got.tempo==137);
 bytes[3]='<';bytes[used-1]=2;
 assert(etch_project_decode(bytes,used,&dst,&pat2,&got)==-1);
 puts("PASS V0.97 OCT chord, single octave actions, mono/poly DSP, PJT11 -> PJT12 storage");
 return 0;
}
