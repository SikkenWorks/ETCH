#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/native_tables.h"
#include <assert.h>
#include <stdio.h>
static struct etch_native_audio s;
static struct etch_native_controls c;
static struct etch_native_pattern p;
static int16_t pcm[128];
static void render(unsigned count){while(count){unsigned n=count>128?128:count;etch_native_render(&s,&c,0,pcm,n);count-=n;}}
static int held(uint16_t note){int total=0;for(int i=0;i<8;i++)if(s.voice[i].active&&s.voice[i].held&&s.voice[i].note==(uint16_t)(4096u+note))++total;return total;}
int main(void){
 struct etch_harmony_settings h={0,0,2,1};uint8_t chord[4];
 assert(etch_quantize_note(61,&h)==60);assert(etch_quantize_note(66,&h)==65);
 assert(etch_chord_notes(60,&h,chord)==3&&chord[0]==60&&chord[1]==64&&chord[2]==67);
 assert(etch_seq_put(&p,0,60,100,3));assert(etch_seq_put(&p,0,64,127,1));
 assert(!etch_seq_put(&p,256,60,100,1));assert(!etch_seq_put(&p,0,60,100,0));
 for(unsigned i=0;i<ETCH_SEQ_NOTES-2;i++)assert(etch_seq_put(&p,0,(uint8_t)(70+i),100,1));
 assert(!etch_seq_put(&p,0,100,100,1));
 etch_pattern_clear(&p);assert(p.steps[0].count==0);
 assert(etch_seq_put(&p,0,60,127,3));assert(etch_seq_put(&p,0,64,80,1));
 assert(etch_seq_put(&p,1,67,99,1));p.steps[1].lock_mask=1u<<ETCH_LOCK_MORPH;p.steps[1].locks[ETCH_LOCK_MORPH]=32767;
 assert(etch_seq_put(&p,255,72,95,1));
 etch_native_reset(&s);c.gain=32767;c.seq_run=1;c.seq_length=256;c.seq_rate=3;c.seq_gate=80;c.tempo=240;c.pattern=&p;
 render(100);assert(held(60)==1&&held(64)==1);
 render(1400);assert(held(60)==1&&held(67)==1&&!held(64));
 assert(s.seq_lock_mask==1&&s.seq_locks[ETCH_LOCK_MORPH]==32767);
 render(1500);assert(held(60)==1);
 render(1500);assert(!held(60));
 c.seq_run=0;render(1);assert(!held(67));
 /* Native authored HOLD playback: no re-attack from HOLD itself; simultaneous
  * fresh same-pitch TRIGGER has a separate gate/voice instance. */
 etch_pattern_clear(&p);
 assert(etch_seq_put(&p,0,60,115,1));
 assert(etch_seq_hold_toggle(&p,1,60)==1);
 assert(etch_seq_hold_toggle(&p,2,60)==1);
 assert(etch_seq_put(&p,1,60,95,1));
 assert(etch_seq_put(&p,1,67,95,1));
 etch_native_reset(&s);c.seq_run=1;c.clock_follow=0;c.seq_length=16;c.seq_rate=3;
 c.seq_gate=80;c.tempo=240;c.pattern=&p;c.harmony.chord=0;
 render(100);assert(held(60)==1);
 render(1400);assert(held(60)==2&&held(67)==1);
 render(1200);assert(held(60)==1&&!held(67));
 render(1550);assert(!held(60));
 c.seq_run=0;render(1);
 /* MIDI clock transport reaches audio core with the realtime status only. */
 etch_native_reset(&s);c.seq_run=1;c.clock_follow=1;render(200);assert(!held(60));
 etch_native_midi(&s,0xfa,0,0);
 for(int i=0;i<25;i++){render(919);etch_native_midi(&s,0xf8,0,0);}
 assert(s.clock_bpm>=110&&s.clock_bpm<=130);assert(s.clock_running);
 etch_native_midi(&s,0xfc,0,0);render(128);assert(!s.clock_running);
 /* MIDI quantization keeps note-off identity while changing pitch. */
 etch_native_reset(&s);c.seq_run=0;c.clock_follow=0;c.harmony=h;c.harmony.chord=0;
 render(1);etch_native_midi(&s,0x90,61,100);
 int found=0;for(int i=0;i<8;i++)if(s.voice[i].held&&s.voice[i].note==189){found=1;assert(s.voice[i].inc==etch_note_inc[60]);}
 assert(found);etch_native_midi(&s,0x80,61,0);for(int i=0;i<8;i++)assert(!(s.voice[i].note==189&&s.voice[i].held));
 puts("Native advanced: 256 poly steps, individual lengths, locks, keylock/chord, MIDI clock/start/stop pass");
}
