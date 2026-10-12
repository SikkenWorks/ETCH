#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdio.h>
static struct etch_native_audio s;
static struct etch_native_controls c;
static struct etch_arp_program arp;
static int16_t out[128];
static uint32_t test_keys=0b10101u;
static void render(unsigned count){while(count){unsigned n=count>128?128:count;etch_native_render(&s,&c,test_keys,out,n);count-=n;}}
static int held(unsigned note){int n=0;for(int i=0;i<8;i++)if(s.voice[i].active&&s.voice[i].held&&s.voice[i].note==4096u+note)n++;return n;}
int main(void){
 etch_native_reset(&s);etch_arp_program_init(&arp);
 arp.length=4;arp.steps[0]=1;arp.steps[1]=255;arp.steps[2]=0;arp.steps[3]=2;
 c.gain=32767;c.arp_mode=6;c.arp_rate=3;c.arp_octaves=1;c.arp_gate=75;c.tempo=240;c.arp_program=&arp;
 render(1);assert(held(55)==1&&!held(53));
 render(1380);assert(!held(55)); /* custom REST: the prior note was released */
 render(1380);assert(held(53)==1&&!held(55));
 render(1380);assert(held(57)==1&&!held(53));
 render(1380);assert(held(55)==1&&!held(57));
 /* -1 cycles the octave below then the played octave; -2 includes two below. */
 test_keys=1u;
 etch_native_reset(&s);c.arp_mode=1;c.arp_program=0;c.arp_octaves=5;c.arp_rate=3;c.arp_gate=100;
 render(1);assert(held(41)==1); /* C below held keyboard note 53 */
 render(1380);assert(held(41)==0&&held(53)==1);
 etch_native_reset(&s);c.arp_octaves=6;
 render(1);assert(held(29)==1);
 render(1380);assert(held(41)==1);
 render(1380);assert(held(53)==1);
 puts("PASS custom ARP and negative octave ranges -1/-2 on native FM1 audio host");
}
