#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static struct etch_native_audio s;
static int16_t out[128];
int main(void){
 struct etch_native_controls c={8192,0,32767,0};
 etch_native_reset(&s);etch_native_render(&s,&c,0,out,128);
 for(int i=0;i<128;i++)assert(!out[i]);
 long energy=0;
 for(int j=0;j<10;j++){etch_native_render(&s,&c,1,out,128);for(int i=0;i<128;i++)energy+=abs(out[i]);}
 assert(energy>100000);assert(s.voice[0].held);
 c.octave=1;etch_native_render(&s,&c,0,out,128);
 for(int j=0;j<10;j++)etch_native_render(&s,&c,0,out,128);
 for(int i=0;i<8;i++)assert(!s.voice[i].active);
 for(int i=0;i<128;i++)assert(!out[i]);
 c.octave=0;c.depth=32767;
 for(int j=0;j<20;j++)etch_native_render(&s,&c,0x7ffffffu,out,128);
 int active=0;for(int i=0;i<8;i++)active+=s.voice[i].active;assert(active==8);
 etch_native_render(&s,&c,0,out,128);for(int j=0;j<20;j++)etch_native_render(&s,&c,0,out,128);
 for(int i=0;i<8;i++)assert(!s.voice[i].active);
 /* V0.89 intentional slew: a gain change must not hard-mute the FIRST
  * sample; after sufficient audio frames the final value is silent. */
 c.gain=0;etch_native_render(&s,&c,1,out,128);
 assert(s.master_gain_q16>0); /* volume is ramping, not stepping */
 for(int j=0;j<100;j++)etch_native_render(&s,&c,1,out,128);
 for(int i=0;i<128;i++)assert(!out[i]);
 assert(etch_native_preview(0,0,0x40000000u)>28000);
 assert(etch_native_preview(32767,0,0x40000000u)!=etch_native_preview(0,0,0x40000000u));
 puts("Native C core: silence, audible output, octave-safe release, voice bounds, max-depth stress and gain mute passed");
}
