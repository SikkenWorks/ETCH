#include "../firmware/fm1-native/native_group_voice.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct etch_key_groups g;etch_key_groups_init(&g);
 assert(etch_key_group_set(&g,0,4,36,47,1));
 assert(etch_key_group_set(&g,1,7,60,72,1));
 struct etch_group_voice_map m;etch_group_voice_reset(&m);
 for(unsigned i=0;i<8;i++){
  unsigned pitch=i<3?40:i<6?64:80;
  uint8_t group=etch_key_group_route(&g,(uint8_t)pitch);
  assert(etch_group_voice_bind(&m,(uint8_t)i,group,(uint16_t)(128+pitch),i+1));
  assert(etch_group_voice_owner(&m,(uint8_t)i,i+1)==group);
 }
 /* Voice stealing: an old note-off cannot release newly bound group 2. */
 assert(etch_group_voice_bind(&m,0,2,192,100));
 assert(etch_group_voice_owner(&m,0,1)==255);
 assert(!etch_group_voice_release(&m,0,168,1));
 assert(!etch_group_voice_release(&m,0,168,100));
 assert(etch_group_voice_owner(&m,0,100)==2);
 assert(etch_group_voice_release(&m,0,192,100));
 assert(etch_group_voice_owner(&m,0,100)==255);
 struct etch_group_capture capture={0};
 assert(!etch_group_should_suppress_note(&capture));
 etch_group_capture_begin(&capture,0);
 assert(etch_group_should_suppress_note(&capture));
 etch_group_capture_note(&capture,&g,20,3);
 assert(etch_group_should_suppress_note(&capture));
 etch_group_capture_cancel(&capture);
 assert(!etch_group_should_suppress_note(&capture));
 etch_group_voice_reset(&m);
 for(unsigned i=0;i<8;i++)assert(etch_group_voice_owner(&m,(uint8_t)i,i+1)==255);
 puts("GROUPS per-voice ownership, stealing, release and capture tests passed");
 return 0;
}
