/* Native allocation and per-voice Group identity, no loss of MIDI note pairing. */
#include <assert.h>
#include <stdio.h>
#include "../firmware/fm1-native/audio_core.h"
static struct etch_native_audio a;
static struct etch_key_groups groups;
static unsigned find(uint16_t id){for(unsigned i=0;i<8;i++)if(a.voice[i].active&&a.voice[i].note==id)return i;return 255u;}
int main(void){
 etch_key_groups_init(&groups);
 assert(etch_key_group_set(&groups,0,4,36,47,1));
 assert(etch_key_group_set(&groups,1,9,60,72,1));
 etch_native_groups_configure(&groups);etch_native_reset(&a);
 etch_native_midi(&a,0x90,40,90);assert(etch_native_voice_group(find(168))==1);
 etch_native_midi(&a,0x91,64,90);assert(etch_native_voice_group(find(320))==2);
 etch_native_midi(&a,0x90,80,90);assert(etch_native_voice_group(find(208))==0);
 etch_native_midi(&a,0x90,64,90);assert(etch_native_voice_group(find(192))==2);
 assert(find(168)!=255u&&find(320)!=255u&&find(208)!=255u);
 etch_native_midi(&a,0x80,40,0);assert(!a.voice[find(168)].held);
 assert(a.voice[find(320)].held);
 for(unsigned i=0;i<8;i++)if(a.voice[i].active&&a.voice[i].held)assert(etch_native_voice_group(i)<=2u);
 etch_native_reset(&a);for(unsigned i=0;i<8;i++)assert(etch_native_voice_group(i)==0u);
 etch_native_groups_configure(0);etch_native_midi(&a,0x90,40,100);
 assert(etch_native_voice_group(find(168))==0u);
 puts("PASS native MIDI routing to 3 group identities, channels, note-off and reset");
}
