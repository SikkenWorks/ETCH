/* V1.00: MONO last-note priority and safe transitions, real sequencer edge. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_tables.h"
#include "../firmware/fm1-native/native_seq_cursor.h"
static struct etch_native_audio a;
static struct etch_native_controls c;
static struct etch_native_pattern pat;
static int16_t out[256];
static void frames(unsigned n,uint32_t keys){while(n){unsigned part=n>128?128:n;etch_native_render_stereo(&a,&c,keys,out,part);n-=part;}}
static void defaults(void){etch_native_reset(&a);c=(struct etch_native_controls){0};c.mono_mode=1;c.env_enabled=1;c.attack_step=0x7fff0000u;c.decay_step=0x7fff0000u;c.sustain_level=32767;c.release_step=0x7fff0000u;c.gain=32767;c.cutoff=8192;frames(1,0);}
int main(void){
 defaults();
 frames(256,1u<<0);assert(a.mono_count==1 && a.voice[0].note==0);
 frames(256,(1u<<0)|(1u<<4));assert(a.mono_count==2 && a.voice[0].note==4);
 frames(256,(1u<<0)|(1u<<4)|(1u<<7));assert(a.mono_count==3 && a.voice[0].note==7);
 frames(256,(1u<<0)|(1u<<4));assert(a.mono_count==2 && a.voice[0].note==4);
 frames(256,1u<<0);assert(a.mono_count==1 && a.voice[0].note==0);
 frames(256,0);assert(a.mono_count==0 && !a.voice[0].held);
 defaults();etch_native_midi(&a,0x90,60,80);uint32_t birth=a.voice[0].birth;
 etch_native_midi(&a,0x90,60,110);assert(a.voice[0].birth>birth&&a.mono_count==2);
 etch_native_midi(&a,0x80,60,0);assert(a.mono_count==1&&a.voice[0].held);
 etch_native_midi(&a,0x80,60,0);assert(a.mono_count==0&&!a.voice[0].held);
 defaults();etch_native_midi(&a,0x90,60,90);etch_native_midi(&a,0x91,65,100);
 assert(a.voice[0].note==128+128+65);
 etch_native_midi(&a,0x81,65,0);assert(a.voice[0].note==128+60);
 etch_native_midi(&a,0xb0,64,127);etch_native_midi(&a,0x80,60,0);
 assert(a.mono_count==1&&!a.voice[0].down&&a.voice[0].held);
 etch_native_midi(&a,0xb0,64,0);assert(a.mono_count==0&&!a.voice[0].held);
 /* A sequencer note that starts 4 samples into a buffer must immediately
  * use its correct phase increment, not the former voice's block rate. */
 defaults();etch_pattern_clear(&pat);
 assert(etch_seq_put(&pat,0,60,127,1));assert(etch_seq_put(&pat,1,72,127,1));
 c.seq_run=1;c.seq_length=8;c.seq_rate=3;c.tempo=240;c.pattern=&pat;
 frames(1,0);assert(a.perf_played&&a.perf_current_step==0);
 assert(a.voice[0].inc==etch_note_inc[60]);
 frames(2756,0); /* 44100*60/240/8 -> 1378 samples: three steps */
 assert(a.perf_current_step==2);
 assert(a.voice[0].note>=4096u);
 /* 256-step sequencer playhead wraps exactly at 255 -> 0. */
 for(unsigned i=0;i<256;i++)assert(etch_seq_actual_step(1,1,1,(uint8_t)i,256,0)==i);
 assert(etch_seq_actual_step(1,1,0,255,256,11)==0);
 puts("PASS V1.00 MONO last-note priority, release fallback, MIDI sustain/retrigger, sample-boundary SEQ, playhead");
}
