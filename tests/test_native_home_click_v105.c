/* Regression: returning to PLAY must not reset an audible voice.
 * The physical HOME-event wiring is checked separately against bringup.c;
 * this host DSP test verifies the audible consequences of not resetting. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/audio_core.h"

static struct etch_native_audio engine;
static struct etch_native_controls controls;
static struct etch_native_pattern pattern;
static int16_t audio[256];

static void play(uint32_t keys, uint32_t frames){
 while(frames){uint32_t n=frames>128u?128u:frames;
  etch_native_render(&engine,&controls,keys,audio,n);frames-=n;}
}
static uint8_t has_active_note(uint16_t id){
 for(unsigned i=0;i<8;i++)if(engine.voice[i].active&&engine.voice[i].note==id)return 1;
 return 0;
}
int main(void){
 etch_native_reset(&engine);
 controls.gain=32767u;controls.attack_step=0x7fff0000u;
 controls.decay_step=1u;controls.sustain_level=32767u;
 controls.release_step=0x7fff0000u; controls.env_enabled=1;
 /* A held key must survive navigation and keep its oscillator phase. */
 play(1u<<8,384u);
 assert(has_active_note(8u));
 uint32_t serial=engine.alloc_serial;
 uint32_t phase=0u;
 for(unsigned i=0;i<8;i++)if(engine.voice[i].active&&engine.voice[i].note==8u){phase=engine.voice[i].p;break;}
 assert(phase!=0u);
 /* HOME changes UI page only. It does not call etch_native_reset(). */
 play(1u<<8,128u);
 assert(engine.keys==(1u<<8));
 assert(engine.alloc_serial==serial);
 assert(has_active_note(8u));
 uint32_t nextphase=0u;
 for(unsigned i=0;i<8;i++)if(engine.voice[i].active&&engine.voice[i].note==8u){nextphase=engine.voice[i].p;break;}
 assert(nextphase!=phase);
 /* Switching to PLAY while the sequencer runs must not stop its notes. */
 etch_native_reset(&engine);
 etch_pattern_clear(&pattern);
 assert(etch_seq_put(&pattern,0,60,127,2));
 controls.pattern=&pattern;
 controls.seq_run=1u;controls.seq_length=16u;
 controls.tempo=120u;controls.seq_rate=3u;controls.seq_gate=100u;
 play(0,300u);
 assert(has_active_note(4096u+60u));
 assert(engine.perf_last_seq==1u);
 serial=engine.alloc_serial;
 play(0,128u); /* the next block after HOME navigation */
 assert(engine.perf_last_seq==1u);
 assert(has_active_note(4096u+60u));
 assert(engine.alloc_serial==serial);
 /* Explicit STOP should RELEASE rather than delete the active waveform. */
 controls.seq_run=0;
 play(0,1);
 assert(has_active_note(4096u+60u));
 for(unsigned i=0;i<8;i++)if(engine.voice[i].active&&engine.voice[i].note==4096u+60u)
  assert(!engine.voice[i].held && engine.voice[i].env>0);
 puts("PASS HOME note continuity, transport preservation and soft STOP release");
 return 0;
}
