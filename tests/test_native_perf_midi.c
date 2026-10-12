#include "../firmware/fm1-native/native_performance_midi.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
static uint32_t packets[4096];static int count,full;
static int emit(uint32_t v,void *unused){(void)unused;if(full)return 0;assert(count<4096);packets[count++]=v;return 1;}
static int has(uint8_t status,uint8_t note){for(int i=0;i<count;i++)if((packets[i]>>8&255)==status && (packets[i]>>16&255)==note)return 1;return 0;}
int main(void){
 struct etch_perf_midi m;struct etch_native_audio audio;struct etch_native_controls c;
 memset(&audio,0,sizeof(audio));memset(&c,0,sizeof(c));etch_perf_midi_reset(&m);
 c.tempo=120;c.seq_run=1;
 audio.voice[0].note=4096+60;audio.voice[0].velocity=90;audio.voice[0].active=audio.voice[0].held=audio.voice[0].down=1;
 etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);
 assert(has(0xfa,0)&&has(0x90,60)&&m.sent[0]);
 count=0;audio.voice[0].note=4096+64;audio.voice[0].velocity=80;
 etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);
 assert(count>=2&&(packets[0]>>8&255)==0x80&&has(0x90,64));
 /* The same pitch on a newly allocated voice is a fresh attack.
  * Emit NOTE OFF then NOTE ON, even if the pitch did not change. */
 count=0;audio.voice[0].birth=11;
 etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);
 assert(count==2 && (packets[0]>>8&255)==0x80 && (packets[1]>>8&255)==0x90);
 assert((packets[0]>>16&255)==64 && (packets[1]>>16&255)==64 && m.birth[0]==11);
 /* If outbound queue is full, preserve original note-on state until
  * a later block can send the OFF/ON pair in order. */
 count=0;full=1;audio.voice[0].birth=12;
 etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);
 assert(m.sent[0] && m.birth[0]==11 && count==0);
 full=0;etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);
 assert(count==2 && (packets[0]>>8&255)==0x80 && (packets[1]>>8&255)==0x90);
 assert(m.birth[0]==12);
 count=0;full=1;audio.voice[0].down=audio.voice[0].held=0;
 etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);assert(m.sent[0]);
 full=0;etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);assert(has(0x80,64)&&!m.sent[0]);
 count=0;for(int i=0;i<20;i++)etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);
 assert(has(0xf8,0));
 count=0;c.seq_run=0;etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);assert(has(0xfc,0));
 count=0;audio.voice[0].note=4096+50;audio.voice[0].active=audio.voice[0].held=audio.voice[0].down=1;
 etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);assert(has(0x90,50));
 count=0;etch_perf_midi_update(&m,&audio,&c,0,128,emit,0);assert(!m.sent[0]&&count==0);
 count=0;etch_perf_midi_update(&m,&audio,&c,1,128,emit,0);assert(has(0x90,50));
 puts("Performance USB-MIDI: same-pitch retrigger, ARP/SEQ voice mirror, note-off priority and backpressure, start/stop/clock, disconnect PASS");
}
