#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdio.h>
static struct etch_native_audio s;
static struct etch_native_controls c;
static int16_t samples[128];
static void render(unsigned count,unsigned keys){while(count){unsigned n=count>128?128:count;etch_native_render(&s,&c,keys,samples,n);count-=n;}}
static int perf_active(void){for(int i=0;i<8;i++)if(s.voice[i].active&&s.voice[i].note>=4096&&s.voice[i].held)return 1;return 0;}
int main(void){
 etch_native_reset(&s);c.gain=32767;c.tempo=120;c.arp_mode=1;c.arp_rate=3;c.arp_octaves=1;
 render(128,3);assert(perf_active());int first=0;for(int i=0;i<8;i++)if(s.voice[i].held&&s.voice[i].note>=4096)first=s.voice[i].note;
 render(2800,3);assert(perf_active());int next=0;for(int i=0;i<8;i++)if(s.voice[i].held&&s.voice[i].note>=4096)next=s.voice[i].note;assert(next!=first);
 render(3000,0);assert(!perf_active());
 etch_native_reset(&s);c.arp_mode=0;c.seq_run=1;c.seq_length=16;c.seq_rate=3;c.seq_gate=70;c.seq_notes[0]=60;c.seq_notes[1]=62;for(int i=2;i<64;i++)c.seq_notes[i]=255;
 render(1,0);assert(perf_active());render(2800,0);assert(perf_active());int id=0;for(int i=0;i<8;i++)if(s.voice[i].held&&s.voice[i].note>=4096)id=s.voice[i].note;assert(id==4158);
 c.seq_run=0;render(100,0);assert(!perf_active());
 puts("Native performance: ARP held-note ordering, automatic gate release, 16-step sequence and STOP passed");
}
