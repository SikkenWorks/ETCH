/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V0.47: platform-independent FM-1 navigation and confirmation safety.
 * Matches the available hardware-app pages. Hardware verification is separate.
 */
#pragma once
#include <stdint.h>

enum etch_ui_page {
 ETCH_UI_PLAY=0, ETCH_UI_ENV=1, ETCH_UI_MOD=2, ETCH_UI_FILTER=3,
 ETCH_UI_FX=4, ETCH_UI_ARP=5, ETCH_UI_SEQ=6, ETCH_UI_PATCH=7,
 ETCH_UI_NAME=8, ETCH_UI_TOOLS=9, ETCH_UI_NOTES=10, ETCH_UI_HARM=11,
 ETCH_UI_LOCKS=12, ETCH_UI_CLOCK=13, ETCH_UI_WFX1=14, ETCH_UI_WFX2=15,
 ETCH_UI_FXA=16, ETCH_UI_FXB=17, ETCH_UI_GRID=18, ETCH_UI_GUIDE=19,
 ETCH_UI_HOLD=20, ETCH_UI_BAKE=21, ETCH_UI_ARP_EDIT=22,
 ETCH_UI_MORPH=23, ETCH_UI_LIBRARY=24, ETCH_UI_SYSTEM=25,
 ETCH_UI_FIFTHS=26, ETCH_UI_PROJECTS=27,
 ETCH_UI_FX_DRIVE=28, ETCH_UI_FX_CHORUS=29,
 ETCH_UI_FX_DELAY=30, ETCH_UI_FX_REVERB=31,
 ETCH_UI_PROJECT_SAVE=32, ETCH_UI_FX_RING=33, ETCH_UI_FX_EQ=34, ETCH_UI_FX_COMP=35, ETCH_UI_FX_TREM=36,
 ETCH_UI_FX_PHASER=37, ETCH_UI_FX_FLANGER=38, ETCH_UI_FX_DECIM=39,
 ETCH_UI_FX_FOLDER=40, ETCH_UI_FX_TAPE=41,
 ETCH_UI_FX_REVERSE=42, ETCH_UI_FX_VIBRATO=43, ETCH_UI_FX_ROTARY=44, ETCH_UI_FX_WAH=45,
 ETCH_UI_PREFX_A=46,ETCH_UI_PREFX_B=47,ETCH_UI_C_FX=48,
 ETCH_UI_MOTION=49, ETCH_UI_EVOLUTION=50, ETCH_UI_PRESETS=51, ETCH_UI_ORGANIC=52, ETCH_UI_SPECTRAL=53, ETCH_UI_LIVING=54, ETCH_UI_MATRIX=55, ETCH_UI_RIFF_LAB=56, ETCH_UI_PATCH_BLEND=57, ETCH_UI_GROUPS=58, ETCH_UI_SCULPT=59, ETCH_UI_WAVE_SEQ=60, ETCH_UI_PERFORMANCE=61, ETCH_UI_FX_BYPASS=62, ETCH_UI_WAVE_AMP=63, ETCH_UI_PAGE_COUNT=64
};

/* V1.35: exactly MOD, C FX and MATRIX are visible in POLY.
 * EVOLUTION, ORGANIC, SPECTRAL, LIVING, PATCH BLEND are MONO-only.
 * Stored preset settings are never deleted when switching voice mode. */
static inline uint8_t etch_ui_mono_only(uint8_t page){
 return (uint8_t)(page==ETCH_UI_EVOLUTION || page==ETCH_UI_ORGANIC ||
                  page==ETCH_UI_SPECTRAL || page==ETCH_UI_LIVING ||
                  page==ETCH_UI_PATCH_BLEND);
}
static inline uint8_t etch_ui_lfo_visible(uint8_t page,uint8_t mono){
 return (uint8_t)(mono || !etch_ui_mono_only(page));
}
static inline uint8_t etch_ui_lfo_next(uint8_t page,uint8_t mono){
 if(!mono){
  if(page==ETCH_UI_MOD)return ETCH_UI_C_FX;
  if(page==ETCH_UI_C_FX)return ETCH_UI_MATRIX;
  if(page==ETCH_UI_MATRIX)return ETCH_UI_WAVE_AMP;
  return ETCH_UI_MOD;
 }
 switch(page){
  case ETCH_UI_MOD: return ETCH_UI_C_FX;
  case ETCH_UI_C_FX: return ETCH_UI_EVOLUTION;
  case ETCH_UI_EVOLUTION: return ETCH_UI_ORGANIC;
  case ETCH_UI_ORGANIC: return ETCH_UI_SPECTRAL;
  case ETCH_UI_SPECTRAL: return ETCH_UI_LIVING;
  case ETCH_UI_LIVING: return ETCH_UI_MATRIX;
  case ETCH_UI_MATRIX: return ETCH_UI_WAVE_AMP;
  case ETCH_UI_WAVE_AMP: return ETCH_UI_PATCH_BLEND;
  case ETCH_UI_PATCH_BLEND: return ETCH_UI_MOD;
  default: return ETCH_UI_MOD;
 }
}
/* Four home-carousel pages in precisely the desktop simulator's order. */
static inline uint8_t etch_ui_is_home(uint8_t page){
 return page==ETCH_UI_PLAY||page==ETCH_UI_MORPH||page==ETCH_UI_FILTER||page==ETCH_UI_LIBRARY;
}
static inline uint8_t etch_ui_home_turn(uint8_t page,int8_t dir){
 static const uint8_t pages[4]={ETCH_UI_PLAY,ETCH_UI_MORPH,ETCH_UI_FILTER,ETCH_UI_LIBRARY};
 if(!dir||!etch_ui_is_home(page))return page;
 for(uint8_t i=0;i<4;i++)if(pages[i]==page)
  return pages[(i+(dir>0?1u:3u))&3u];
 return page;
}
/* SEQ exposes the actual native inspectors; not all have separate stock buttons. */
/* Simulator parity: SEQ cycles GRID -> STEP -> HARMONY. Additional editors
 * remain reachable via ALGORITHM, not hidden/removed from the firmware. */
static inline uint8_t etch_ui_seq_next(uint8_t page){
 if(page==ETCH_UI_SEQ)return ETCH_UI_NOTES;
 if(page==ETCH_UI_NOTES)return ETCH_UI_HARM;
 if(page==ETCH_UI_HARM)return ETCH_UI_WAVE_SEQ;
 return ETCH_UI_SEQ;
}
static inline uint8_t etch_ui_seq_context(uint8_t page){
 return page==ETCH_UI_SEQ||page==ETCH_UI_NOTES||page==ETCH_UI_HARM||
        page==ETCH_UI_GRID||page==ETCH_UI_HOLD||page==ETCH_UI_LOCKS||page==ETCH_UI_WAVE_SEQ||
        page==ETCH_UI_BAKE||page==ETCH_UI_CLOCK;
}
/* ALGORITHM gives access to the advanced GRID/HOLD/LOCK/BAKE/TEMPO editors.
 * The three main SEQ pages remain the ordinary SEQ-button workflow. */
static inline uint8_t etch_ui_seq_inspector(uint8_t page,int32_t turns){
 static const uint8_t pages[]={ETCH_UI_SEQ,ETCH_UI_NOTES,ETCH_UI_HARM,
   ETCH_UI_GRID,ETCH_UI_HOLD,ETCH_UI_LOCKS,ETCH_UI_BAKE,ETCH_UI_CLOCK,ETCH_UI_WAVE_SEQ};
 uint8_t idx=0;
 for(uint8_t i=0;i<9u;i++)if(pages[i]==page){idx=i;break;}
 int32_t next=((int32_t)idx+(turns%9))%9;
 if(next<0)next+=9;
 return pages[next];
}
static inline uint8_t etch_ui_arp_next(uint8_t page){
 return page==ETCH_UI_ARP?ETCH_UI_ARP_EDIT:page==ETCH_UI_ARP_EDIT?ETCH_UI_CLOCK:page==ETCH_UI_CLOCK?ETCH_UI_RIFF_LAB:ETCH_UI_ARP;
}
static inline uint8_t etch_ui_fx_next(uint8_t page){
 return page==ETCH_UI_FX?ETCH_UI_FXA:page==ETCH_UI_FXA?ETCH_UI_FXB:ETCH_UI_FX;
}
/* V0.73: Dedicated FX workflow. One detent moves exactly one FX page.
 * The navigation is separate from ENV/FILTER so their physical buttons and
 * existing patches keep their original behaviour. */
static inline uint8_t etch_ui_is_fx_editor(uint8_t page){
 return (page>=ETCH_UI_FX_DRIVE && page<=ETCH_UI_FX_REVERB)||page==ETCH_UI_FX_RING||page==ETCH_UI_FX_EQ||
   (page>=ETCH_UI_FX_COMP&&page<=ETCH_UI_FX_WAH);
}
static inline uint8_t etch_ui_fx_select(uint8_t page,int32_t turns){
 if(!etch_ui_is_fx_editor(page))return ETCH_UI_FX_DRIVE;
 static const uint8_t fx_pages[17]={ETCH_UI_FX_DRIVE,ETCH_UI_FX_CHORUS,ETCH_UI_FX_DELAY,ETCH_UI_FX_REVERSE,
    ETCH_UI_FX_REVERB,ETCH_UI_FX_RING,ETCH_UI_FX_EQ,ETCH_UI_FX_COMP,
    ETCH_UI_FX_TREM,ETCH_UI_FX_VIBRATO,ETCH_UI_FX_PHASER,ETCH_UI_FX_FLANGER,
    ETCH_UI_FX_DECIM,ETCH_UI_FX_FOLDER,ETCH_UI_FX_TAPE,ETCH_UI_FX_ROTARY,ETCH_UI_FX_WAH};
 int32_t old=0;
 for(uint8_t k=0;k<17u;k++)if(fx_pages[k]==page){old=k;break;}
 int32_t index=(old+turns%17)%17;
 if(index<0)index+=17;
 return fx_pages[index];
}
static inline const char *etch_ui_title(uint8_t page){
 static const char *const titles[ETCH_UI_PAGE_COUNT]={
  "PLAY","ENV","MOD","FILTER","FX","ARP","SEQ","PATCH","NAME","TOOLS",
  "NOTES","HARM","LOCKS","CLOCK","WFX 1","WFX 2","FX A","FX B",
  "GRID","GUIDE","HOLD","BAKE","ARP EDIT","MORPH","LIBRARY","SYSTEM","FIFTHS","PROJECTS",
  "DRIVE","CHORUS","DELAY","REVERB","PROJECT SAVE","RING MOD","CHANNEL EQ",
  "COMPRESSOR","TREMOLO","PHASER","FLANGER","DECIMATOR","WAVEFOLDER","TAPE INSTABILITY",
  "REVERSE DELAY","VIBRATO","ROTARY SPEAKER","AUTO-WAH",
  "WAVE A FX","WAVE B FX","WAVE C FX MOD","MOTION","EVOLUTION","PRESETS","ORGANIC","SPECTRAL","LIVING","MOD MATRIX","RIFF LAB","PATCH BLEND","GROUPS","SCULPT","WAVE SEQ","PERFORMANCE","FX BYPASS","WAVE AMP"
 };
 return page<ETCH_UI_PAGE_COUNT?titles[page]:"ERROR";
}
/* Dedicated global-button navigation. Third press preserves diagnostics,
 * USB status and recovery readouts rather than hiding the SYSTEM page. */
static inline uint8_t etch_ui_glo_next(uint8_t page){
 if(page==ETCH_UI_GUIDE)return ETCH_UI_FIFTHS;
 if(page==ETCH_UI_FIFTHS)return ETCH_UI_SYSTEM;
 if(page==ETCH_UI_MOTION)return ETCH_UI_SYSTEM;
 return ETCH_UI_GUIDE;
}
/* SEQ's chord builder may share the GUIDE page ID but must not advance
 * GLO before the first PERFORMANCE GUIDE press. */
static inline uint8_t etch_ui_glo_route(uint8_t page,uint8_t performance_guide){
 return page==ETCH_UI_GUIDE&&!performance_guide?ETCH_UI_GUIDE:etch_ui_glo_next(page);
}
/* Cancel all armed destructive operations when crossing a page or edit-mode
 * boundary. A second REC is never permitted to complete on another page. */
struct etch_ui_arms {
 uint8_t wave_load, wave_save, patch_save, bake, wave_fx_bake, seq_clear;
};
static inline uint8_t etch_ui_cancel_on_transition(uint8_t old_page,uint8_t new_page,
 uint8_t old_draw,uint8_t new_draw,struct etch_ui_arms *arms){
 if(old_page==new_page&&old_draw==new_draw)return 0;
 arms->wave_load=arms->wave_save=arms->patch_save=0;
 arms->bake=arms->wave_fx_bake=arms->seq_clear=0;
 return 1;
}
/* Which of A, B or C opens in DRAW from a performance page. */
static inline uint8_t etch_ui_draw_target(uint8_t source_page,uint8_t morph_focus,
                                           uint8_t library_target,uint8_t previous){
 if(source_page==ETCH_UI_MOD)return 2;
 if(source_page==ETCH_UI_MORPH)return morph_focus<3?morph_focus:0;
 if(source_page==ETCH_UI_LIBRARY)return library_target<3?library_target:0;
 return previous<3?previous:0;
}
/* System diagnostics are read-only; tab 7 provides runtime expressive controls. */
static inline uint8_t etch_ui_system_tab(uint8_t old,int8_t direction){
 return (uint8_t)((old+7u+(direction>0?1u:direction<0?6u:0u))%7u);
}

/* The circle advances in fifths C-G-D-A-E-B-F#-C#-G#-D#-A#-F.
 * No project-format change: selecting a root edits the existing harmony root. */
static inline uint8_t etch_ui_fifths_pitch(uint8_t index){
 static const uint8_t fifths[12]={0,7,2,9,4,11,6,1,8,3,10,5};
 return fifths[index%12u];
}
static inline uint8_t etch_ui_fifths_index(uint8_t pitch){
 for(uint8_t i=0;i<12;i++)if(etch_ui_fifths_pitch(i)==pitch%12u)return i;
 return 0;
}
static inline uint8_t etch_ui_fifths_move(uint8_t pitch,int32_t turns){
 int32_t at=(int32_t)etch_ui_fifths_index(pitch)+(turns%12);
 at=(at%12+12)%12;return etch_ui_fifths_pitch((uint8_t)at);
}


/* V1.71 user-facing workflow. SIMPLE changes page navigation only;
 * existing DSP, patch serialization, USB, project storage and all 62
 * editors remain unchanged and are still available in ADVANCED.
 * The mode is runtime-only and defaults to SIMPLE on every boot. */
enum etch_ui_workflow { ETCH_WORKFLOW_SIMPLE=0, ETCH_WORKFLOW_ADVANCED=1 };
static inline uint8_t etch_ui_workflow_change(uint8_t workflow,int32_t turns){
 if(!turns)return workflow?ETCH_WORKFLOW_ADVANCED:ETCH_WORKFLOW_SIMPLE;
 return turns>0?ETCH_WORKFLOW_ADVANCED:ETCH_WORKFLOW_SIMPLE;
}
static inline uint8_t etch_ui_home_workflow_page(uint8_t page,uint8_t workflow){
 if(workflow)return etch_ui_is_home(page);
 return (uint8_t)(etch_ui_is_home(page)||page==ETCH_UI_FX);
}
static inline uint8_t etch_ui_home_workflow_turn(uint8_t page,int8_t dir,uint8_t workflow){
 if(workflow)return etch_ui_home_turn(page,dir);
 static const uint8_t pages[5]={ETCH_UI_PLAY,ETCH_UI_MORPH,ETCH_UI_FILTER,ETCH_UI_FX,ETCH_UI_LIBRARY};
 for(uint8_t i=0;i<5u;i++)if(page==pages[i])
  return (uint8_t)(!dir?page:pages[(i+(dir>0?1u:4u))%5u]);
 return page;
}
static inline uint8_t etch_ui_workflow_fx(uint8_t page,uint8_t workflow){
 if(!workflow)return ETCH_UI_FX;
 if(page==ETCH_UI_FX)return ETCH_UI_FX_DRIVE;
 if(page==ETCH_UI_PREFX_A)return ETCH_UI_PREFX_B;
 if(page==ETCH_UI_PREFX_B)return ETCH_UI_FX;
 if(etch_ui_is_fx_editor(page))return ETCH_UI_PREFX_A;
 return ETCH_UI_FX;
}
static inline uint8_t etch_ui_workflow_lfo(uint8_t page,uint8_t mono,uint8_t workflow){
 return workflow?etch_ui_lfo_next(page,mono):ETCH_UI_MOD;
}
static inline uint8_t etch_ui_workflow_seq(uint8_t page,uint8_t workflow){
 if(workflow)return etch_ui_seq_next(page);
 return page==ETCH_UI_SEQ?ETCH_UI_NOTES:ETCH_UI_SEQ;
}
static inline uint8_t etch_ui_workflow_arp(uint8_t page,uint8_t workflow){
 return workflow?etch_ui_arp_next(page):ETCH_UI_ARP;
}
static inline uint8_t etch_ui_workflow_glo(uint8_t page,uint8_t workflow,uint8_t guide){
 return workflow?etch_ui_glo_route(page,guide):(page==ETCH_UI_SYSTEM?ETCH_UI_PLAY:ETCH_UI_SYSTEM);
}
/* Existing 7 system views are preserved, the eighth is WORKFLOW. */
static inline uint8_t etch_ui_system_tab_workflow(uint8_t old,int8_t direction){
 if(!direction)return old%8u;
 return (uint8_t)(((int32_t)(old%8u)+8+(direction>0?1:-1))%8);
}
/* The 106px header box is 13 monospace 8px glyphs.  Longer legacy titles
 * get a short, unambiguous screen header; underlying page names are intact. */
static inline const char *etch_ui_short_title(uint8_t page){
 switch(page){
  case ETCH_UI_FX_TAPE:return "TAPE FX";
  case ETCH_UI_FX_ROTARY:return "ROTARY";
  case ETCH_UI_FX_REVERSE:return "REV DELAY";
  case ETCH_UI_FX_FOLDER:return "WAVEFOLDER";
  case ETCH_UI_FX_COMP:return "COMPRESSOR";
  case ETCH_UI_FX_EQ:return "CHANNEL EQ";
  case ETCH_UI_C_FX:return "WAVE C FX";
  case ETCH_UI_WAVE_AMP:return "WAVE AMP";
  case ETCH_UI_PROJECT_SAVE:return "PROJ SAVE";
  default:return etch_ui_title(page);
 }
}
/* Format a value for a 60px parameter cell; no glyph may bleed into
 * a neighbouring cell. Oversized values are visibly marked instead of
 * silently showing a misleading truncated number. */
static inline void etch_ui_cell_number(uint32_t value,char out[11],uint8_t width){
 char reverse[11];uint8_t n=0,i=0;
 do{reverse[n++]=(char)('0'+value%10u);value/=10u;}while(value && n<10u);
 if(!width){out[0]=0;return;}
 if(n>width){
  for(i=0;i+1u<width&&i<9u;i++)out[i]='9';
  out[i++]='+';out[i]=0;return;
 }
 for(i=0;i<n;i++)out[i]=reverse[n-1u-i];
 out[n]=0;
}

/* Keep the fourth knob strictly inside the right LCD edge.
 * V1.70 erroneously passed right=241, so label_box discarded K4. */
static inline uint32_t etch_ui_footer_right(uint8_t i){
 uint32_t r=4u+((uint32_t)i+1u)*60u-3u;
 return r>240u?240u:r;
}

/* V1.72: every existing page has a direct Advanced catalog route.
 * No removal or renumbering of page IDs or serializer fields. */
static inline uint8_t etch_ui_catalog_turn(uint8_t selected,int32_t delta){
 int32_t next=((int32_t)(selected%ETCH_UI_PAGE_COUNT)+(delta%ETCH_UI_PAGE_COUNT))%ETCH_UI_PAGE_COUNT;
 if(next<0)next+=ETCH_UI_PAGE_COUNT;
 return (uint8_t)next;
}
static inline uint8_t etch_ui_catalog_allowed(uint8_t selected,uint8_t mono){
 return (uint8_t)(selected<ETCH_UI_PAGE_COUNT && etch_ui_lfo_visible(selected,mono));
}
static inline uint8_t etch_ui_catalog_needs_draw(uint8_t selected){
 return (uint8_t)(selected==ETCH_UI_TOOLS||selected==ETCH_UI_WFX1||
                  selected==ETCH_UI_WFX2||selected==ETCH_UI_SCULPT);
}
/* Calling code must initialize page-specific state before entering a page. */
