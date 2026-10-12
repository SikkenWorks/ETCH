/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "native_ui_navigation.h"
/* Context-only: the existing physical confirmation key acts as YES on
 * PROJECTS. Its other actions are completely unchanged outside this page.
 * A project is opened with one confirmation, a NEW project only when there
 * is an empty slot. Destructive DELETE still needs its separate chord. */
/* FM1 matrix top-row button 3 is physical SEL; button 13 is REC. */
#define ETCH_PROJECT_SEL_BUTTON 3u
enum etch_project_ui_choice {ETCH_PROJECT_NONE=0,ETCH_PROJECT_OPEN=1,ETCH_PROJECT_NEW=2};
static inline enum etch_project_ui_choice etch_project_sel_choice(
    uint8_t page,uint8_t slot,uint8_t present,uint32_t pressed){
 if(page!=ETCH_UI_PROJECTS||!(pressed&(1u<<ETCH_PROJECT_SEL_BUTTON)))return ETCH_PROJECT_NONE;
 if(slot==4u)return (present&15u)==15u?ETCH_PROJECT_NONE:ETCH_PROJECT_NEW;
 if(slot<4u&&(present&(1u<<slot)))return ETCH_PROJECT_OPEN;
 return ETCH_PROJECT_NONE;
}
/* SEL+SAVE is a deliberate double-chord for DELETE. A SAVE press while
 * SEL is held must never be stolen by the global project-save shortcut. */
static inline uint8_t etch_project_delete_chord(uint8_t page,uint32_t buttons,uint32_t pressed){
 const uint32_t chord=(1u<<13)|(1u<<9);
 return page==ETCH_UI_PROJECTS && (buttons&chord)==chord && (pressed&chord)!=0u;
}
static inline uint8_t etch_project_allow_save_shortcut(uint8_t page,uint32_t buttons){
 return page!=ETCH_UI_PROJECTS || !(buttons&(1u<<13));
}
#define ETCH_PROJECT_UI_TITLE_Y 87u
#define ETCH_PROJECT_UI_FIRST_Y 106u
#define ETCH_PROJECT_UI_ROW_STEP 18u
#define ETCH_PROJECT_UI_NEW_Y 178u
#define ETCH_PROJECT_UI_HINT_Y 205u
#define ETCH_PROJECT_UI_CONFIRM_Y 222u
