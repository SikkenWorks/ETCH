/* ETCH V1.43: HOME+FX and HOME+SAVE shortcuts. No HOME double-tap. */
#pragma once
#include <stdint.h>
enum etch_home_shortcut { ETCH_SHORTCUT_NONE=0,ETCH_SHORTCUT_GROUPS=1,ETCH_SHORTCUT_PROJECTS=2,ETCH_SHORTCUT_PERFORMANCE=3 };
struct etch_home_shortcuts {uint8_t latched;};
static inline enum etch_home_shortcut etch_home_shortcuts_scan(struct etch_home_shortcuts *s,uint32_t buttons,uint32_t pressed){
 const uint32_t home=1u<<8,fx=1u<<2,save=1u<<9,arp=1u<<10;
 if(!(buttons&home)){s->latched=0u;return ETCH_SHORTCUT_NONE;}
 if(s->latched)return ETCH_SHORTCUT_NONE;
 if((buttons&(home|arp))==(home|arp) && (pressed&(home|arp))){s->latched=1u;return ETCH_SHORTCUT_PERFORMANCE;}
 if((buttons&(home|save))==(home|save) && (pressed&(home|save))){s->latched=1u;return ETCH_SHORTCUT_PROJECTS;}
 if((buttons&(home|fx))==(home|fx) && (pressed&(home|fx))){s->latched=1u;return ETCH_SHORTCUT_GROUPS;}
 return ETCH_SHORTCUT_NONE;
}
