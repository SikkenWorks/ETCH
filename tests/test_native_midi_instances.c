/* V0.42: independent same-pitch MIDI note instances and FIFO note-off */
#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdio.h>
static struct etch_native_audio audio;
static unsigned count(uint16_t id,int down,int held){
 unsigned n=0;
 for(unsigned i=0;i<ETCH_NATIVE_VOICES;i++){
  const struct etch_native_voice *v=&audio.voice[i];
  if(v->active&&v->note==id&&(down<0||v->down==down)&&(held<0||v->held==held))++n;
 }
 return n;
}
int main(void){
 etch_native_reset(&audio);
 uint16_t id=(uint16_t)(128+60);
 etch_native_midi(&audio,0x90,60,50);
 etch_native_midi(&audio,0x90,60,100);
 assert(count(id,1,1)==2);
 unsigned old=9,newer=9;
 for(unsigned i=0;i<8;i++)if(audio.voice[i].active&&audio.voice[i].note==id){
  if(audio.voice[i].velocity==50)old=i;
  if(audio.voice[i].velocity==100)newer=i;
 }
 assert(old<8&&newer<8&&old!=newer);
 etch_native_midi(&audio,0x80,60,0);
 assert(!audio.voice[old].down&&!audio.voice[old].held);
 assert(audio.voice[newer].down&&audio.voice[newer].held);
 etch_native_midi(&audio,0x90,60,0);
 assert(count(id,1,1)==0);
 /* Stale note-off must not kill an unrelated new voice. */
 etch_native_midi(&audio,0x80,60,0);
 etch_native_midi(&audio,0x90,62,90);
 assert(count(190,1,1)==1);
 etch_native_midi(&audio,0x80,60,0);
 assert(count(190,1,1)==1);
 /* Sustain holds exactly the released instance; note-off pairs once. */
 etch_native_reset(&audio);
 etch_native_midi(&audio,0xb0,64,127);
 etch_native_midi(&audio,0x90,60,80);
 etch_native_midi(&audio,0x90,60,90);
 etch_native_midi(&audio,0x80,60,0);
 assert(count(id,0,1)==1&&count(id,1,1)==1);
 etch_native_midi(&audio,0x80,60,0);
 assert(count(id,0,1)==2);
 etch_native_midi(&audio,0xb0,64,0);
 assert(count(id,-1,1)==0);
 /* Same pitch on another channel has a separate identity. */
 etch_native_reset(&audio);
 etch_native_midi(&audio,0x90,60,96);
 etch_native_midi(&audio,0x91,60,108);
 assert(count(id,1,1)==1&&count((uint16_t)(id+128),1,1)==1);
 etch_native_midi(&audio,0x80,60,0);
 assert(count((uint16_t)(id+128),1,1)==1);
 /* A stolen note has no matching voice left; extra offs are harmless. */
 etch_native_reset(&audio);
 for(unsigned n=60;n<68;n++)etch_native_midi(&audio,0x90,(uint8_t)n,100);
 etch_native_midi(&audio,0x90,68,100);
 etch_native_midi(&audio,0x80,60,0);
 assert(count(196,1,1)==1);
 etch_native_midi(&audio,0xb0,123,0);
 for(unsigned i=0;i<8;i++)assert(!audio.voice[i].down);
 puts("PASS MIDI overlapping instances, FIFO note-off, sustain, channels and stealing");
 return 0;
}
