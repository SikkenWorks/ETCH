/* ETCH V0.44 two same-pitch SEQ triggers with independent authored HOLD gates. */
#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdio.h>
static struct etch_native_audio a;
static struct etch_native_controls c;
static struct etch_native_pattern p;
static int16_t buffer[128];
static void run(unsigned frames){while(frames){unsigned n=frames>128?128:frames;etch_native_render(&a,&c,0,buffer,n);frames-=n;}}
static int held(uint8_t note){int total=0;for(unsigned i=0;i<8;i++)if(a.voice[i].active&&a.voice[i].held&&a.voice[i].note==(uint16_t)(4096+note))total++;return total;}
int main(void){
 etch_pattern_clear(&p);
 assert(etch_seq_put(&p,0,60,100,1));
 assert(etch_seq_put(&p,1,60,110,1));
 assert(etch_seq_hold_link(&p,16,1,60,0)==1);
 assert(etch_seq_hold_link(&p,16,2,60,0)==1);
 assert(etch_seq_hold_link(&p,16,2,60,1)==1);
 c.tempo=240;c.seq_rate=3;c.seq_run=1;c.seq_length=16;c.seq_gate=75;c.gain=32767;c.pattern=&p;
 etch_native_reset(&a);
 run(1);assert(held(60)==1);
 run(1378);assert(held(60)==2);
 run(1378);assert(held(60)==2); /* same pitch, different source and length */
 run(1378);assert(held(60)==0); /* both bounded; no stuck voice after the pattern */
 c.seq_run=0;run(128);assert(held(60)==0);
 puts("PASS actual FM1 native audio: two simultaneous same-pitch sequencer HOLD voices, independent releases, STOP");
}
