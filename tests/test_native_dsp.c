/* Native behavior tests, independent of target hardware timing. */
#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
static struct etch_native_audio s;
static struct etch_native_controls c;
static int16_t out[128];
static void run(unsigned samples,unsigned keys){while(samples){unsigned n=samples>128?128:samples;etch_native_render(&s,&c,keys,out,n);samples-=n;}}
int main(void){
 c.gain=32767;c.env_enabled=1;c.attack_step=0x7fff0000u/100;c.decay_step=0x7fff0000u/100;c.sustain_level=16384;c.release_step=0x7fff0000u/200;
 etch_native_reset(&s);run(50,1);assert(s.voice[0].env>16000&&s.voice[0].env<17000);run(52,1);assert(s.voice[0].stage==1);run(60,1);assert(s.voice[0].stage==2&&s.voice[0].env==16384);
 run(50,0);assert(s.voice[0].env>7800&&s.voice[0].env<8500);run(55,0);assert(!s.voice[0].active);
 /* Real note retrigger restarts attack, pitch bend stays channel-local. */
 etch_native_reset(&s);etch_native_midi(&s,0x90,69,127);etch_native_midi(&s,0x91,69,100);run(1,0);uint32_t base=s.voice[0].inc;
 etch_native_midi(&s,0xe0,127,127);run(1,0);assert(s.bend[0]>65536&&s.bend[1]==65536);assert(s.voice[0].p>2*base&&s.voice[1].p==2*base);
 etch_native_midi(&s,0xb0,121,0);assert(s.bend[0]==65536);
 etch_native_midi(&s,0xe1,0,0);assert(s.bend[1]<65536);etch_native_midi_panic(&s);assert(s.bend[1]==65536);
 /* Keyed x16 and free rates use independent modulator phase. */
 etch_native_reset(&s);c.mod_ratio=11;run(1,1);assert(s.voice[0].q==s.voice[0].inc*16u);c.mod_mode=1;c.mod_inc=12345;uint32_t q=s.voice[0].q;run(1,1);assert(s.voice[0].q-q==12345);
 /* TEMPO mode uses the same free-running mod-inc published by the UI. */
 etch_native_reset(&s);c.mod_mode=2;c.mod_inc=1234;run(1,1);assert(s.voice[0].q==1234);
 /* Both modulation targets and all filter states stay bounded under stress. */
 c.mod_mode=0;c.depth=32767;c.resonance=32767;c.drive=32767;c.cutoff=8192;c.attack_step=0x7fff0000u;c.sustain_level=32767;
 for(unsigned target=0;target<2;target++)for(unsigned type=0;type<4;type++)for(unsigned ratio=0;ratio<12;ratio++){
  etch_native_reset(&s);c.mod_target=target;c.filter_type=type;c.mod_ratio=ratio;run(16000,0x7ffffffu);
  for(unsigned i=0;i<8;i++){assert(abs(s.voice[i].low)<=262144);assert(abs(s.voice[i].band)<=262144);}
  run(256,0);for(unsigned i=0;i<8;i++)assert(!s.voice[i].active);
 }
 /* 10-second envelope steps retain sub-Q15 resolution and eventually release. */
 etch_native_reset(&s);c.filter_type=c.drive=0;c.attack_step=4867;c.release_step=4867;run(100,1);assert(s.voice[0].env>0&&s.voice[0].env<100);run(101,0);assert(!s.voice[0].active);
 puts("Native ADSR stage timing, channel-local bend, C ratio/free phase, filter/drive stress and long-stage resolution passed");
}
