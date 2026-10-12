#include "../firmware/fm1-native/native_music_labels.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 const char *names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
 for(unsigned i=0;i<12;i++)assert(strcmp(etch_note_name(i),names[i])==0);
 assert(strcmp(etch_note_name(15),"D#")==0);
 assert(strcmp(etch_mode_name(0),"MAJOR")==0);
 assert(strcmp(etch_mode_name(5),"MINOR")==0);
 assert(strcmp(etch_mode_name(1),"DORIAN")==0);
 for(unsigned i=0;i<9;i++)assert(strlen(etch_mode_short(i))<=7);
 for(unsigned i=0;i<3;i++)assert(strlen(etch_lock_name(i))<=5);

 /* The visible keyboard is pitch-class absolute, NOT root relative. */
 for(unsigned root=0;root<12;root++){
  for(unsigned col=0;col<ETCH_GUIDE_KEY_COUNT;col++){
   unsigned pc=etch_guide_pitch_at_column((uint8_t)col);
   assert(pc==col);
   assert(strcmp(etch_note_name((uint8_t)pc),names[col])==0);
   assert(etch_guide_column_x((uint8_t)col)==6u+19u*col);
   assert(etch_guide_column_x((uint8_t)col)+ETCH_GUIDE_KEY_WIDTH<=240u);
   assert(etch_guide_is_accidental((uint8_t)pc)==
       (pc==1||pc==3||pc==6||pc==8||pc==10));
  }
  assert(etch_guide_pitch_at_column((uint8_t)root)==root);
 }
 assert(strcmp(etch_note_name(etch_guide_pitch_at_column(1)),"C#")==0);
 assert(strcmp(etch_note_name(etch_guide_pitch_at_column(11)),"B")==0);
 puts("PASS V0.83 fixed C-to-B chromatic key labels for all 12 roots, bounded footer labels");
}
