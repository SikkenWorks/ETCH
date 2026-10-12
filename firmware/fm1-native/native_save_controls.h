/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 SAVE: K1 slot, K2 cursor (skipped position means space),
 * K3 letter, K4 counter-clockwise backspace. All editing is staged.
 */
#pragma once
#include <stdint.h>
static inline uint8_t etch_save_position(uint8_t pos,int32_t delta){
 int32_t next=(int32_t)pos+delta;
 return (uint8_t)(next<0?0:next>11?11:next);
}
static inline void etch_save_backspace(char name[13],uint8_t *pos,int32_t turn){
 if(!name||!pos||turn>=0)return;
 while(turn++<0){
  if(*pos>0)--*pos;
  name[*pos]=' ';
 }
 name[12]=0;
}
static inline uint8_t etch_save_has_name(const char name[13]){
 if(!name)return 0;
 for(uint8_t i=0;i<12u;i++)if(name[i]!=' ' && name[i]!=0)return 1u;
 return 0u;
}
static inline uint8_t etch_save_toast_visible(uint32_t now,uint32_t until){
 return until!=0u && (int32_t)(until-now)>0;
}
