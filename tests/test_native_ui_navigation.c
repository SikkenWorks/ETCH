/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/native_ui_navigation.h"
#include "../firmware/fm1-native/native_ui_wave.h"
#include "../firmware/fm1-native/native_project_ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 uint8_t page=ETCH_UI_PLAY;
 const uint8_t expected[]={23,3,24,0,23,3,24,0};
 for(int i=0;i<8;i++){page=etch_ui_home_turn(page,1);assert(page==expected[i]);
  assert(page==etch_ui_wave_page_cycle(i==0?0:expected[i-1],1));}
 for(int i=0;i<4;i++)page=etch_ui_home_turn(page,-1);
 assert(page==0);
 assert(etch_ui_home_turn(ETCH_UI_SEQ,1)==ETCH_UI_SEQ);
 page=ETCH_UI_SEQ;
 for(int i=0;i<3;i++)page=etch_ui_seq_next(page);
 assert(page==ETCH_UI_SEQ);
 assert(etch_ui_seq_next(ETCH_UI_SEQ)==ETCH_UI_NOTES);
 assert(etch_ui_seq_next(ETCH_UI_NOTES)==ETCH_UI_HARM);
 assert(etch_ui_seq_next(ETCH_UI_HARM)==ETCH_UI_SEQ);
 assert(etch_ui_seq_context(ETCH_UI_HOLD));
 assert(etch_ui_seq_inspector(ETCH_UI_SEQ,3)==ETCH_UI_GRID);
 assert(etch_ui_seq_inspector(ETCH_UI_SEQ,-1)==ETCH_UI_CLOCK);
 assert(etch_ui_seq_inspector(ETCH_UI_CLOCK,1)==ETCH_UI_SEQ);
 page=ETCH_UI_ARP;for(int i=0;i<4;i++)page=etch_ui_arp_next(page);assert(page==ETCH_UI_ARP);
 assert(etch_ui_arp_next(ETCH_UI_CLOCK)==ETCH_UI_RIFF_LAB);
 page=ETCH_UI_FX;for(int i=0;i<3;i++)page=etch_ui_fx_next(page);assert(page==ETCH_UI_FX);
 const uint8_t fx[]={ETCH_UI_FX_DRIVE,ETCH_UI_FX_CHORUS,ETCH_UI_FX_DELAY,ETCH_UI_FX_REVERSE,ETCH_UI_FX_REVERB,ETCH_UI_FX_RING,ETCH_UI_FX_EQ,
 ETCH_UI_FX_COMP,ETCH_UI_FX_TREM,ETCH_UI_FX_VIBRATO,ETCH_UI_FX_PHASER,ETCH_UI_FX_FLANGER,ETCH_UI_FX_DECIM,ETCH_UI_FX_FOLDER,ETCH_UI_FX_TAPE,ETCH_UI_FX_ROTARY,ETCH_UI_FX_WAH};
 page=ETCH_UI_FX_DRIVE;
 assert(etch_ui_is_fx_editor(page));
 for(int i=1;i<17;i++){page=etch_ui_fx_select(page,1);assert(page==fx[i%17]);}
 assert(etch_ui_fx_select(ETCH_UI_FX_DRIVE,-1)==ETCH_UI_FX_WAH);
 assert(etch_ui_fx_select(ETCH_UI_FX_REVERB,1)==ETCH_UI_FX_RING);
 assert(etch_ui_fx_select(ETCH_UI_FX_DELAY,-18)==ETCH_UI_FX_CHORUS);
 assert(etch_ui_fx_select(ETCH_UI_PLAY,3)==ETCH_UI_FX_DRIVE);
 assert(!etch_ui_is_fx_editor(ETCH_UI_ENV));
 assert(etch_ui_glo_next(ETCH_UI_PLAY)==ETCH_UI_GUIDE);
 assert(etch_ui_glo_next(ETCH_UI_GUIDE)==ETCH_UI_FIFTHS);
 assert(etch_ui_glo_next(ETCH_UI_FIFTHS)==ETCH_UI_SYSTEM);
 assert(etch_ui_glo_next(ETCH_UI_SYSTEM)==ETCH_UI_GUIDE);
 assert(etch_ui_glo_route(ETCH_UI_GUIDE,0)==ETCH_UI_GUIDE); /* CHORD BUILDER -> SCALE GUIDE */
 assert(etch_ui_glo_route(ETCH_UI_GUIDE,1)==ETCH_UI_FIFTHS);
 assert(etch_ui_glo_route(ETCH_UI_PLAY,0)==ETCH_UI_GUIDE);
 /* Project SEL is contextual. Outside PROJECTS nothing is hijacked;
  * empty/invalid/full selections never trigger a load or new project. */
 assert(etch_project_sel_choice(ETCH_UI_PLAY,1,0x2,(1u<<3))==ETCH_PROJECT_NONE);
 assert(etch_project_sel_choice(ETCH_UI_PROJECT_SAVE,1,0x2,(1u<<3))==ETCH_PROJECT_NONE);
 assert(etch_project_sel_choice(ETCH_UI_PROJECTS,1,0x2,0)==ETCH_PROJECT_NONE);
 assert(etch_project_sel_choice(ETCH_UI_PROJECTS,1,0x2,(1u<<13))==ETCH_PROJECT_NONE); /* REC is not SEL */
 assert(etch_project_sel_choice(ETCH_UI_PROJECTS,1,0x2,(1u<<3))==ETCH_PROJECT_OPEN);
 assert(etch_project_sel_choice(ETCH_UI_PROJECTS,1,0x0,(1u<<3))==ETCH_PROJECT_NONE);
 assert(etch_project_sel_choice(ETCH_UI_PROJECTS,4,0x7,(1u<<3))==ETCH_PROJECT_NEW);
 assert(etch_project_sel_choice(ETCH_UI_PROJECTS,4,0xf,(1u<<3))==ETCH_PROJECT_NONE);
 assert(etch_project_sel_choice(ETCH_UI_PROJECTS,5,0xf,(1u<<3))==ETCH_PROJECT_NONE);
 assert(etch_project_delete_chord(ETCH_UI_PLAY,(1u<<13)|(1u<<9),(1u<<9))==0);
 assert(etch_project_delete_chord(ETCH_UI_PROJECTS,(1u<<13)|(1u<<9),(1u<<9))==1);
 assert(etch_project_delete_chord(ETCH_UI_PROJECTS,(1u<<13),(1u<<13))==0);
 assert(!etch_project_allow_save_shortcut(ETCH_UI_PROJECTS,(1u<<13)|(1u<<9)));
 assert(etch_project_allow_save_shortcut(ETCH_UI_PROJECTS,(1u<<9)));
 assert(etch_project_allow_save_shortcut(ETCH_UI_PLAY,(1u<<13)|(1u<<9)));
 assert(ETCH_PROJECT_UI_FIRST_Y + 3u*ETCH_PROJECT_UI_ROW_STEP + 7u < ETCH_PROJECT_UI_NEW_Y);
 assert(ETCH_PROJECT_UI_NEW_Y+7u < ETCH_PROJECT_UI_HINT_Y);
 assert(ETCH_PROJECT_UI_HINT_Y+7u < ETCH_PROJECT_UI_CONFIRM_Y);
 assert(!etch_ui_is_fx_editor(ETCH_UI_FILTER));
 assert(!etch_ui_is_fx_editor(ETCH_UI_ENV));
 assert(strcmp(etch_ui_title(ETCH_UI_ENV),"ENV")==0);
 assert(strcmp(etch_ui_title(ETCH_UI_FX_DRIVE),"DRIVE")==0);
 assert(strcmp(etch_ui_title(ETCH_UI_FX_CHORUS),"CHORUS")==0);
 assert(strcmp(etch_ui_title(ETCH_UI_FX_DELAY),"DELAY")==0);
 assert(strcmp(etch_ui_title(ETCH_UI_FX_REVERB),"REVERB")==0);
 for(int i=0;i<ETCH_UI_PAGE_COUNT;i++)assert(strlen(etch_ui_title(i))>0);
 assert(strcmp(etch_ui_title(ETCH_UI_SYSTEM),"SYSTEM")==0);
 assert(strcmp(etch_ui_title(ETCH_UI_PAGE_COUNT),"ERROR")==0);
 assert(strcmp(etch_ui_title(ETCH_UI_FIFTHS),"FIFTHS")==0);
 assert(strcmp(etch_ui_title(ETCH_UI_PROJECTS),"PROJECTS")==0);
 assert(strcmp(etch_ui_title(ETCH_UI_PROJECT_SAVE),"PROJECT SAVE")==0);
 static const uint8_t expected_circle[12]={0,7,2,9,4,11,6,1,8,3,10,5};
 for(int i=0;i<12;i++){
  assert(etch_ui_fifths_pitch(i)==expected_circle[i]);
  assert(etch_ui_fifths_index(expected_circle[i])==i);
  assert(etch_ui_fifths_move(expected_circle[i],1)==expected_circle[(i+1)%12]);
  assert(etch_ui_fifths_move(expected_circle[i],-1)==expected_circle[(i+11)%12]);
  assert(etch_ui_fifths_move(expected_circle[i],24)==expected_circle[i]);
 }
 assert(etch_ui_draw_target(ETCH_UI_MOD,0,0,0)==2);
 assert(etch_ui_draw_target(ETCH_UI_MORPH,2,0,0)==2);
 assert(etch_ui_draw_target(ETCH_UI_LIBRARY,0,1,0)==1);
 assert(etch_ui_draw_target(ETCH_UI_PLAY,0,0,1)==1);
 assert(etch_ui_draw_target(ETCH_UI_MORPH,255,0,0)==0);
 assert(etch_ui_system_tab(0,1)==1 && etch_ui_system_tab(1,1)==2 && etch_ui_system_tab(2,1)==3);
 assert(etch_ui_system_tab(3,1)==4 && etch_ui_system_tab(4,1)==5 && etch_ui_system_tab(5,1)==0);
 assert(etch_ui_system_tab(0,-1)==5 && etch_ui_system_tab(1,-1)==0);
 struct etch_ui_arms arms={1,1,1,1,1,1};
 assert(!etch_ui_cancel_on_transition(24,24,0,0,&arms));assert(arms.wave_load);
 assert(etch_ui_cancel_on_transition(24,23,0,0,&arms));
 assert(!arms.wave_load&&!arms.wave_save&&!arms.patch_save&&!arms.bake&&!arms.wave_fx_bake&&!arms.seq_clear);
 arms.wave_load=1;assert(etch_ui_cancel_on_transition(0,0,1,0,&arms)&&!arms.wave_load);
 printf("PASS V0.77 all 33 page IDs, FX four-page SELECT, GLO GUIDE/FIFTHS/SYSTEM, ADSR via ENV, no destructive page transitions\n");
}
