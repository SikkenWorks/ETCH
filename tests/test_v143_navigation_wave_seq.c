#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_home_shortcuts.h"
#include "../firmware/fm1-native/native_ui_navigation.h"
#include "../firmware/fm1-native/native_help_chapters.h"
#include "../firmware/fm1-native/native_wave_seq.h"
int main(void){
 struct etch_home_shortcuts h={0};
 assert(etch_home_shortcuts_scan(&h,1u<<8,1u<<8)==ETCH_SHORTCUT_NONE);
 assert(etch_home_shortcuts_scan(&h,(1u<<8)|(1u<<2),1u<<2)==ETCH_SHORTCUT_GROUPS);
 assert(etch_home_shortcuts_scan(&h,(1u<<8)|(1u<<2),0)==ETCH_SHORTCUT_NONE);
 assert(etch_home_shortcuts_scan(&h,0,0)==ETCH_SHORTCUT_NONE);
 assert(etch_home_shortcuts_scan(&h,(1u<<8)|(1u<<9),(1u<<8)|(1u<<9))==ETCH_SHORTCUT_PROJECTS);
 assert(etch_home_shortcuts_scan(&h,(1u<<8)|(1u<<9),1u<<9)==ETCH_SHORTCUT_NONE);
 assert(etch_home_shortcuts_scan(&h,0,0)==ETCH_SHORTCUT_NONE);
 assert(etch_home_shortcuts_scan(&h,(1u<<8)|(1u<<2)|(1u<<9),(1u<<8)|(1u<<2)|(1u<<9))==ETCH_SHORTCUT_PROJECTS);
 assert(ETCH_HELP_CHAPTERS>=18);
 assert(etch_help_turn(0,1)==1);
 assert(etch_help_turn(0,-1)==ETCH_HELP_CHAPTERS-1);
 assert(etch_help_turn(ETCH_HELP_CHAPTERS-1,1)==0);
 for(unsigned c=0;c<ETCH_HELP_CHAPTERS;c++){
  assert(etch_help_chapters[c].title[0]);
  assert(strlen(etch_help_chapters[c].title)<=23);
  for(int j=0;j<5;j++){assert(etch_help_chapters[c].line[j]);assert(strlen(etch_help_chapters[c].line[j])<=27);}
 }
 struct etch_seq_step step={0};
 assert(!etch_wave_seq_enabled(&step));
 etch_wave_seq_set_position(&step,0);assert(etch_wave_seq_enabled(&step)&&etch_wave_seq_mode(&step)==0);
 etch_wave_seq_set_position(&step,100);assert(etch_wave_seq_mode(&step)==2);
 etch_wave_seq_set_position(&step,40);assert(etch_wave_seq_mode(&step)==1&&etch_wave_seq_position(&step)<=41);
 etch_wave_seq_select(&step,-1);assert(etch_wave_seq_mode(&step)==0);
 etch_wave_seq_select(&step,2);assert(etch_wave_seq_mode(&step)==2);
 etch_wave_seq_toggle(&step,-1);assert(!etch_wave_seq_enabled(&step));
 etch_wave_seq_toggle(&step,1);assert(etch_wave_seq_enabled(&step));
 assert(etch_ui_seq_next(ETCH_UI_HARM)==ETCH_UI_WAVE_SEQ);
 assert(etch_ui_seq_next(ETCH_UI_WAVE_SEQ)==ETCH_UI_SEQ);
 assert(etch_ui_seq_inspector(ETCH_UI_SEQ,-1)==ETCH_UI_WAVE_SEQ);
 puts("PASS V1.43 HOME+FX HOME+SAVE chapters bidirectional 4-stage SEQ wave morph locks");
}
