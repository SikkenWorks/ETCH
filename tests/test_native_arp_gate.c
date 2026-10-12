/* V0.41 gate parameter must affect live ARP duration, not merely UI state. */
#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdio.h>
static struct etch_native_audio s;
static struct etch_native_controls c;
static int16_t pcm[128];
static void advance(unsigned n){while(n){unsigned take=n<128?n:128;etch_native_render(&s,&c,1u,pcm,take);n-=take;}}
static int held_arp(void){for(int i=0;i<8;i++)if(s.voice[i].note>=4096&&s.voice[i].active&&s.voice[i].held)return 1;return 0;}
int main(void){
 etch_native_reset(&s);c.arp_mode=1;c.arp_rate=3;c.arp_octaves=1;c.tempo=240;c.arp_gate=25;c.gain=32767;
 advance(200);assert(held_arp());advance(230);assert(!held_arp());
 etch_native_reset(&s);c.arp_gate=75;
 advance(430);assert(held_arp());advance(620);assert(!held_arp());
 puts("PASS ARP gate affects native voice duration at 25% and 75%");
 return 0;
}
