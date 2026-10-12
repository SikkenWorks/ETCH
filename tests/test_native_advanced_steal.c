/* ETCH V0.71: ranked multi-note stealing, velocity, repeated pitch and sustain. */
#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static struct etch_native_audio s;
static int exists(unsigned id){for(unsigned i=0;i<8;i++)if(s.voice[i].active&&s.voice[i].note==id)return 1;return 0;}
static struct etch_native_voice *find(unsigned id){for(unsigned i=0;i<8;i++)if(s.voice[i].active&&s.voice[i].note==id)return &s.voice[i];return 0;}
int main(void){
 etch_native_reset(&s);
 for(unsigned n=60;n<68;n++)etch_native_midi(&s,0x90,(uint8_t)n,90);
 assert(s.steal==0);
 /* New ninth note replaces an older held note, not a freshly triggered voice. */
 etch_native_midi(&s,0x90,68,90);
 assert(s.steal==1 && !exists(188) && exists(196));
 etch_native_reset(&s);
 for(unsigned n=60;n<68;n++)etch_native_midi(&s,0x90,(uint8_t)n,90);
 struct etch_native_voice *quiet=find(192),*loud=find(193);
 assert(quiet && loud);
 quiet->env=2000; loud->env=26000;
 for(unsigned n=60;n<68;n++)if(n!=64 && n!=65)find(128+n)->env=20000;
 etch_native_midi(&s,0x90,74,90);
 assert(!exists(192) && exists(202)); /* audible quietest */
 etch_native_reset(&s);
 for(unsigned n=60;n<68;n++)etch_native_midi(&s,0x90,(uint8_t)n,95);
 etch_native_midi(&s,0xb0,64,127); /* sustain ON */
 etch_native_midi(&s,0x80,62,0);  /* key UP, sustained */
 assert(find(190) && !find(190)->down && find(190)->held);
 etch_native_midi(&s,0x90,75,95);
 assert(!exists(190) && exists(203));
 etch_native_reset(&s);
 for(unsigned n=60;n<68;n++)etch_native_midi(&s,0x90,(uint8_t)n,95);
 etch_native_midi(&s,0x90,64,95); /* full-poly same note prefers retrigger */
 assert(!exists(188) == 0); /* unrelated held low note survives */
 unsigned same=0;for(unsigned i=0;i<8;i++)if(s.voice[i].active && s.voice[i].note==192)same++;
 assert(same==1 && s.steal==1);
 /* repeated pressure never exceeds the hard 8-voice limit */
 for(unsigned n=69;n<127;n++)etch_native_midi(&s,0x90,(uint8_t)n,127);
 unsigned active=0;for(unsigned i=0;i<8;i++)active+=s.voice[i].active?1:0;
 assert(active==8);
 puts("PASS advanced stealing released/sustain/quietest/retrigger/bounded 8");
}
