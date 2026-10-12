/* V0.54: transactional staging and continuous audio over patch changes. */
#include "../firmware/fm1-native/native_patch_swap.h"
#include "../firmware/fm1-native/audio_core.h"
#include "../firmware/fm1-native/wave_editor.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static struct etch_patch_swap swapper;
static struct etch_pattern_bank archive;
static struct etch_native_pattern authored;
static struct etch_native_audio audio;
static struct etch_wave_editor edit;
static struct etch_native_bank waves[2];
static int16_t samples[ETCH_NATIVE_FRAMES*2];
static unsigned active_voices(const struct etch_native_audio *s){
 unsigned n=0;for(unsigned i=0;i<ETCH_NATIVE_VOICES;i++)n+=s->voice[i].active!=0;return n;
}
static uint64_t energy(void){
 uint64_t e=0;for(unsigned i=0;i<ETCH_NATIVE_FRAMES*2;i++)e+=(samples[i]<0?-(int32_t)samples[i]:samples[i]);return e;
}
int main(void){
 etch_pattern_bank_clear(&archive);
 etch_pattern_clear(&authored);
 assert(etch_seq_put(&authored,0,60,100,12));
 assert(etch_pattern_bank_capture(&archive,0,&authored)==0);
 etch_pattern_clear(&authored);
 assert(etch_seq_put(&authored,0,64,100,15));
 assert(etch_pattern_bank_capture(&archive,1,&authored)==0);
 assert(etch_patch_prepare(&swapper,&archive,0)==1);
 assert(etch_patch_active(&swapper)->steps[0].count==0);
 struct etch_native_pattern *first=etch_patch_commit(&swapper);
 assert(first->steps[0].notes[0].note==60);
 assert(etch_patch_prepare(&swapper,&archive,1)==1);
 assert(etch_patch_active(&swapper)==first);
 assert(first->steps[0].notes[0].note==60);
 struct etch_native_pattern *second=etch_patch_commit(&swapper);
 assert(second!=first && second->steps[0].notes[0].note==64);
 assert(first->steps[0].notes[0].note==60);
 /* A corrupted archive must not commit or mutate the currently audible pattern. */
 uint16_t saved=archive.lengths[1];archive.lengths[1]=65535;
 assert(etch_patch_prepare(&swapper,&archive,1)<0);
 assert(etch_patch_active(&swapper)==second);
 assert(second->steps[0].notes[0].note==64);
 archive.lengths[1]=saved;
 /* Eight held voices, 256-sample waveform crossfade, no voice/DSP reset. */
 etch_editor_init(&edit);
 etch_editor_compile(&edit,&waves[0]);
 for(unsigned i=0;i<128;i++)edit.points[0][i]=(int16_t)(edit.points[0][i]/2);
 etch_editor_compile(&edit,&waves[1]);
 struct etch_native_controls c={0};
 c.gain=32767;c.bank=&waves[0];c.pattern=first;
 etch_native_reset(&audio);
 for(unsigned i=0;i<5;i++)etch_native_render_stereo(&audio,&c,0xffu,samples,ETCH_NATIVE_FRAMES);
 assert(active_voices(&audio)==8 && energy()>1000);
 uint32_t previous_counter=audio.alloc_serial;
 c.bank=&waves[1];c.pattern=second;c.morph=12000;c.depth=3000;
 etch_native_render_stereo(&audio,&c,0xffu,samples,ETCH_NATIVE_FRAMES);
 assert(active_voices(&audio)==8 && energy()>1000);
 assert(audio.bank_fade>0 && audio.alloc_serial==previous_counter);
 etch_native_render_stereo(&audio,&c,0xffu,samples,ETCH_NATIVE_FRAMES);
 assert(audio.bank_fade==0 && energy()>1000);
 assert(audio.alloc_serial==previous_counter);
 /* Repeated nonblocking patch and wavetable transitions must never panic,
  * reset the 8 held voices or output a completely silent audio block. */
 for(unsigned iteration=0;iteration<64;iteration++){
  uint8_t slot=(uint8_t)(iteration&1u);
  assert(etch_patch_prepare(&swapper,&archive,slot)==1);
  c.pattern=etch_patch_commit(&swapper);
  c.bank=&waves[slot];
  for(unsigned j=0;j<2;j++){
   etch_native_render_stereo(&audio,&c,0xffu,samples,ETCH_NATIVE_FRAMES);
   assert(energy()>1000 && active_voices(&audio)==8);
  }
  assert(audio.alloc_serial==previous_counter);
 }
 puts("I03 transactional pattern prepare/commit, corrupt archive refusal, 8-voice nonstopping bank crossfade PASS");
 return 0;
}
