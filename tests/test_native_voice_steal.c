/* V0.41 regression: prefer free/released voices; old active notes survive. */
#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdio.h>
static struct etch_native_audio s;
static int present(unsigned id){for(unsigned j=0;j<8;j++)if(s.voice[j].active&&s.voice[j].note==id)return 1;return 0;}
int main(void){
 etch_native_reset(&s);
 for(unsigned note=60;note<68;note++)etch_native_midi(&s,0x90,(uint8_t)note,95);
 for(unsigned note=60;note<68;note++)assert(present(128u+note));
 /* When all notes are equally held, the oldest should be displaced. */
 etch_native_midi(&s,0x90,68,95);
 assert(!present(188)&&present(196));
 for(unsigned note=61;note<68;note++)assert(present(128u+note));
 /* Release a quiet voice, then replace *that* rather than a held note. */
 etch_native_midi(&s,0x80,64,0);
 int found=0;for(unsigned j=0;j<8;j++)if(s.voice[j].note==192&&!s.voice[j].held)found=1;
 assert(found);
 etch_native_midi(&s,0x90,69,95);
 assert(!present(192)&&present(197));
 for(unsigned note=61;note<68;note++)if(note!=64)assert(present(128u+note));
 /* Repeated 8+ note pressure never allocates more than eight slots. */
 for(unsigned note=70;note<127;note++)etch_native_midi(&s,0x90,(uint8_t)note,127);
 unsigned active=0;for(unsigned j=0;j<8;j++)if(s.voice[j].active)active++;
 assert(active==8);
 puts("PASS V0.41 deterministic held/release-aware 8-voice stealing");
 return 0;
}
