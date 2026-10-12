/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.20: HOME hold + named preset browser, no flash writes.
 * Pure selection and gesture policy shared with the host regression test.
 */
#pragma once
#include <stdint.h>
#define ETCH_PRESETS_HOLD_MS 1500u
#define ETCH_PRESETS_ROWS 7u
#define ETCH_PRESETS_ROW_Y 82u
#define ETCH_PRESETS_ROW_H 18u

enum etch_home_action { ETCH_HOME_NONE=0, ETCH_HOME_SHORT=1, ETCH_HOME_PRESETS=2 };
struct etch_home_gesture { uint32_t start_ms; uint8_t pending, long_fired, chorded; };
/* Release short HOME to navigate; holding it opens browser once. Suppress
 * accidental menu entry for HOME+PLAY, HOME+SAVE, or diagnostics chords.
 * Unsigned elapsed subtraction handles the 32-bit uptime rollover. */
static inline enum etch_home_action etch_home_gesture_poll(struct etch_home_gesture *g,
 uint8_t home_down, uint8_t other_button_down, uint32_t now_ms){
 if(home_down){
  if(!g->pending){g->start_ms=now_ms;g->pending=1u;g->long_fired=0u;g->chorded=0u;}
  if(other_button_down)g->chorded=1u;
  if(!g->long_fired&&!g->chorded && (uint32_t)(now_ms-g->start_ms)>=ETCH_PRESETS_HOLD_MS){
   g->long_fired=1u;return ETCH_HOME_PRESETS;
  }
  return ETCH_HOME_NONE;
 }
 if(g->pending){
  uint8_t short_tap=(uint8_t)(!g->long_fired&&!g->chorded);
  *g=(struct etch_home_gesture){0};
  return short_tap?ETCH_HOME_SHORT:ETCH_HOME_NONE;
 }
 return ETCH_HOME_NONE;
}
static inline uint8_t etch_presets_next_valid(uint8_t selected,int32_t steps,uint32_t mask){
 if(!steps||!mask)return selected;
 uint32_t count=steps>0?(uint32_t)steps:(uint32_t)(-(int64_t)steps);
 if(count>32u)count=32u;
 int direction=steps>0?1:-1;
 for(uint32_t i=0;i<count;i++){
  uint8_t next=selected;
  for(uint8_t k=0;k<32;k++){
   next=(uint8_t)((next+(direction>0?1u:31u))&31u);
   if(mask&(1u<<next))break;
  }
  selected=next;
 }
 return selected;
}
static inline uint8_t etch_presets_name_copy(char dest[13],const char source[12]){
 uint8_t present=0;
 for(uint8_t i=0;i<12;i++){
  char c=source[i];
  if(!c) { for(;i<12;i++)dest[i]=' ';break; }
  if((unsigned char)c<32u||(unsigned char)c>126u)c=' ';
  dest[i]=c;if(c!=' ')present=1u;
 }
 dest[12]='\0';return present;
}
