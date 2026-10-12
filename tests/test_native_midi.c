#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/boot_guard.h"
#include <assert.h>
#include <stdio.h>
static struct etch_native_audio s;
static struct etch_native_controls c={8192,0,32767,0,0};
static int16_t out[128];
static int held(unsigned id){int n=0;for(int i=0;i<8;i++)n+=s.voice[i].note==id&&s.voice[i].held;return n;}
static int active(void){int n=0;for(int i=0;i<8;i++)n+=s.voice[i].active;return n;}
static void settle(void){for(int i=0;i<20;i++)etch_native_render(&s,&c,0,out,128);}
static long energy(unsigned velocity){etch_native_reset(&s);etch_native_midi(&s,0x90,60,velocity);long e=0;for(int j=0;j<8;j++){etch_native_render(&s,&c,0,out,128);for(int i=0;i<128;i++)e+=out[i]<0?-out[i]:out[i];}return e;}
int main(void){
 struct boot_guard b={0};assert(!etch_boot_begin(&b));assert(!etch_boot_begin(&b));assert(etch_boot_begin(&b));
 etch_boot_stable(&b,1);assert(etch_boot_begin(&b));assert(b.failed==2); /* SAFE stays safe through reset. */
 b.magic=0;assert(!etch_boot_begin(&b));etch_boot_stable(&b,0);assert(!etch_boot_begin(&b));
 assert(energy(127)>energy(32)*3);
 etch_native_reset(&s);etch_native_midi(&s,0x90,60,100);etch_native_midi(&s,0x91,60,100);
 assert(held(188)==1&&held(316)==1);etch_native_midi(&s,0x80,60,0);assert(!held(188)&&held(316));
 etch_native_midi(&s,0x91,60,0);settle();assert(!active());
 etch_native_midi(&s,0x90,64,100);etch_native_midi(&s,0xb0,64,127);etch_native_midi(&s,0x80,64,0);settle();assert(held(192));
 etch_native_midi(&s,0xb1,64,0);assert(held(192));etch_native_midi(&s,0xb0,64,0);settle();assert(!active());
 etch_native_midi(&s,0x90,60,100);etch_native_midi(&s,0x90,60,100);assert(held(188)==2);etch_native_midi(&s,0x80,60,0);assert(held(188)==1);etch_native_midi(&s,0x80,60,0);settle();assert(!active());
 etch_native_midi(&s,0x90,60,100);etch_native_midi(&s,0x91,60,100);etch_native_midi(&s,0xb0,120,0);assert(!held(188)&&held(316));
 etch_native_midi(&s,0xb1,123,0);settle();assert(!active());
 etch_native_midi(&s,0x90,60,100);etch_native_midi(&s,0xb0,64,127);etch_native_midi(&s,0xb0,123,0);assert(held(188));etch_native_midi(&s,0xb0,121,0);settle();assert(!active());
 etch_native_render(&s,&c,1,out,128);etch_native_midi(&s,0x90,0,127);etch_native_midi(&s,0x9f,127,127);etch_native_midi_panic(&s);
 assert(held(0)&&!held(128)&&!held(2175));
 etch_native_midi(&s,0x90,128,127);assert(!held(256));
 puts("MIDI: channel separation, velocity, sustain, retrigger, CC120/121/123, panic isolation, note bounds; boot SAFE latch passed");
}
