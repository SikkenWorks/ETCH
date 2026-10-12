/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH native R2 bring-up. Uses the Felucca HAL by Leo Kuroshita.
 * Native lab candidate with MIDI, ROM softkey and optional MIDI OTA writes.
 */
#include <stdint.h>
#include "hal/fm1_cc.h"
#include "hal/fm1_time.h"
#include "hal/fm1_sys.h"
#include "hal/fm1_irq.h"
#include "hal/fm1_guard.h"
#include "hal/fm1_input.h"
#include "hal/fm1_timer.h"
#include "hal/fm1_audio.h"
#include "hal/fm1_adc.h"
#include "hal/fm1_lcd_hw.h"
#include "libc.c"
#include "native_ui_wave.h"
#include "native_ui_navigation.h"
#include "native_seq_parity.h"
#include "native_flash_probe.h"
#include "native_project_ui.h"
#include "native_music_labels.h"
#include "native_save_controls.h"
#include "native_ui_scope_policy.h"
#include "native_led_policy.h"
#include "native_fx_controls.h"
#include "native_encoder_steps.h"
#include "native_live_controls_v117.h"
#include "lcd.c"
/* Preserve Felucca's original lcd.c byte-for-byte (audit and OTA safety).
 * ETCH's taller offscreen UI pages need an application-only segmented blit:
 * do not ask the SPI DMA engine for an unproven >64-KiB transfer. Separate
 * complete ST7789 windows keep the RAMWR command boundary deterministic. */
static void etch_lcd_blit_segmented(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *px){
 if(!w||!h||x>=240u||y>=240u||w>240u-x||h>240u-y)return;
 uint32_t max_rows=56000u/(w*2u);
 if(!max_rows)max_rows=1u;
 for(uint32_t row=0;row<h;){
  uint32_t rows=h-row;
  if(rows>max_rows)rows=max_rows;
  lcd_window(x,y+row,x+w-1u,y+row+rows-1u);
  lcd_data(px+row*w,w*rows*2u);
  row+=rows;
 }
}
#define lcd_blit etch_lcd_blit_segmented
/* One atomic LCD DMA transfer for the waveform region, rather than wiping
 * LCD RAM and issuing 240 individual pixel commands. Memory pool headroom
 * has been verified by the target linker. RGB565 is byte-swapped for SPI. */
#define ETCH_SCOPE_MAX_HEIGHT 173u
static uint16_t etch_scope_canvas[240u*ETCH_SCOPE_MAX_HEIGHT] __attribute__((section(".pool"),aligned(4)));
static uint8_t etch_scope_active;
/* Non-PLAY pages build their entire mutable content in RAM. Nested
 * legacy scope begin/commit calls must not flush a half-rendered page. */
static uint8_t etch_panel_composite;
static uint16_t etch_scope_origin,etch_scope_height;
static void etch_ui_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t color){
 if(!w||!h||x>=240u||y>=240u)return;
 if(x+w>240u)w=240u-x;
 if(y+h>240u)h=240u-y;
 if(etch_scope_active){
  const uint32_t top=etch_scope_origin;
  const uint32_t bottom=top+etch_scope_height;
  if(etch_panel_composite){
   /* Never bypass RAM with a partially clipped glyph/rectangle. That would
    * expose an early SPI write during page composition. */
   if(y>=bottom||y+h<=top)return;
   if(y<top){uint32_t skip=top-y;y=top;h-=skip;}
   if(y+h>bottom)h=bottom-y;
  }
  if(y>=top&&y+h<=bottom){
   const uint16_t swapped=(uint16_t)((color>>8)|(color<<8));
   uint16_t *q=etch_scope_canvas+(y-top)*240u+x;
   for(uint32_t row=0;row<h;row++){
    for(uint32_t col=0;col<w;col++)q[col]=swapped;
    q+=240u;
   }
   return;
  }
 }
 lcd_fill(x,y,w,h,color);
}
static void etch_scope_begin(uint16_t y,uint16_t height){
 if(etch_panel_composite)return; /* A legacy scope is part of the pending page. */
 etch_scope_origin=y;etch_scope_height=height;etch_scope_active=1;
}
static void etch_scope_commit(void){
 if(etch_panel_composite)return; /* Commit once after every label and value. */
 etch_scope_active=0;
 lcd_blit(0,etch_scope_origin,240u,etch_scope_height,etch_scope_canvas);
 lcd_sync(); /* canvas may not be touched while SPI DMA is in flight */
}
/* 174 lines (66..239) fit inside the linker-checked Felucca RAM pool.
 * All fills/text on these pages are drawn offscreen, including erasures.
 * This replaces dozens of visible SPI rectangles with one consistent blit.
 * No DMA/flash/audio ISR change: the operation remains on the UI thread. */
static void etch_panel_begin(uint16_t top){
 etch_scope_origin=top;
 etch_scope_height=(uint16_t)(240u-top);
 etch_scope_active=1u;
 etch_panel_composite=1u;
 /* Clear only RAM: the actual LCD remains untouched until commit. */
 uint32_t n=240u*(uint32_t)etch_scope_height;
 for(uint32_t i=0;i<n;i++)etch_scope_canvas[i]=0u;
}
static void etch_panel_commit(void){
 etch_panel_composite=0u;
 etch_scope_commit();
}
#define lcd_fill etch_ui_fill
/* Draw a connected waveform into the RAM scope surface. Adjacent X columns
 * are connected at every Y even for steep peaks and editor discontinuities.
 * Input is already clipped by etch_ui_wave_display_y. No LCD DMA calls here. */
static void etch_scope_stroke(uint32_t x,uint32_t prior,uint32_t current,uint16_t color){
 if(x>239u||!etch_scope_active)return;
 uint32_t lo=prior<current?prior:current,hi=prior>current?prior:current;
 uint32_t upper=etch_scope_origin+etch_scope_height;
 if(lo<etch_scope_origin)lo=etch_scope_origin;
 if(hi>=upper)hi=upper-1u;
 if(lo<=hi)etch_ui_fill(x,lo,1u,hi-lo+1u,color);
}
extern uint32_t _bss_start[],_bss_end[],_pool_start[],_pool_end[];
extern uint32_t _data_load[],_data_start[],_data_end[],_rt_load[];
extern void isr_timer5(void);
volatile uint32_t etch_ms;
/* TIMER5 remains higher priority than ALNK0, as in Felucca. */
#include "native_irq_bridge.h"
static struct etch_irq_bridge native_irq_bridge;
static volatile uint8_t native_in_audio;
static volatile uint32_t native_nested_ticks, native_nested_count;
#define ETCH_NATIVE_USB 1
#ifndef ETCH_NATIVE_UAC
#define ETCH_NATIVE_UAC 0
#endif
#define FELUCCA_UAC ETCH_NATIVE_UAC
#ifndef ETCH_NATIVE_UAC_48K
#define ETCH_NATIVE_UAC_48K 0
#endif
#define FELUCCA_UAC_48K ETCH_NATIVE_UAC_48K /* host-selectable FIR resampling; device unvalidated */
#define HALF_FRAMES 128u /* Must match ETCH_NATIVE_FRAMES, verified after audio_core.h */
#define ETCH_NATIVE_PANEL_MIDI 1
#ifndef ETCH_NATIVE_OTA
#define ETCH_NATIVE_OTA 0
#endif
#define FELUCCA_OTA ETCH_NATIVE_OTA
#define FELUCCA_ID "FM-1_1020"
#define ETCH_UI_VERSION "V120"
#define FELUCCA_OTA_DRYRUN 0
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#include "usb.c"
#include "boot_guard.h"
#include "native_recovery_policy.h"
#include "native_capacity.h"
static uint8_t native_safe,native_boot_muted;
static enum etch_recovery_reason native_recovery_reason;
struct boot_guard etch_boot __attribute__((section(".noinit")));
void fm1_timer5_irq(void){
 uint32_t start=fm1_ticks();
 fm1_timer5_ack();
 fm1_input_tick();
 uint8_t nested=native_in_audio;
 uint8_t job=etch_irq_bridge_tick(&native_irq_bridge,start,nested,&etch_ms);
 fm1_ms=etch_ms;
#if ETCH_NATIVE_UAC
 if(job==ETCH_IRQ_UAC_SERVICE)uac_service(); /* EP4-only; Felucca's nested USB audio path */
#endif
 if(job==ETCH_IRQ_USB_POLL)usb_poll(); /* non-nested only */
 if(nested){++native_nested_count;native_nested_ticks+=fm1_ticks()-start;}
}
#include "audio_target.c"
_Static_assert(HALF_FRAMES == ETCH_NATIVE_FRAMES, "USB ring must match DMA half frames");
#include "native_factory.h"
static void fm1_fault(const fm1_crash_t *c){(void)c;etch_boot_audio_fault(&etch_boot);fm1_audio_stop();native_stop();fm1_irq_off();fm1_enter_uboot();}
static void text_etch(void){
 static const uint8_t glyph[4][7]={{31,16,16,30,16,16,31},{31,4,4,4,4,4,4},{15,16,16,16,16,16,15},{17,17,17,31,17,17,17}};
 for(uint32_t g=0;g<4;g++)for(uint32_t y=0;y<7;y++)for(uint32_t x=0;x<5;x++)if(glyph[g][y]&(16u>>x))lcd_fill(12+g*24+x*4,18+y*4,4,4,0xD7F0);
}
static uint8_t etch_label_scale=1;
static void label(const char *text,uint32_t x,uint32_t y,uint16_t color){
 static const char names[]="PLAYDRWBCSVEFIOKMNUT0123456789XGHZQJ-_#:+/%.<>";
 static const uint8_t glyph[46][7]={
  {30,17,17,30,16,16,16},{16,16,16,16,16,16,31},{14,17,17,31,17,17,17},{17,17,10,4,4,4,4},
  {30,17,17,17,17,17,30},{30,17,17,30,20,18,17},{17,17,17,21,21,21,10},
  {30,17,17,30,17,17,30},{15,16,16,16,16,16,15},
  {15,16,16,14,1,1,30},{17,17,17,17,17,10,4},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
  {14,4,4,4,4,4,14},{14,17,17,17,17,17,14},{17,18,20,24,20,18,17},{17,27,21,21,17,17,17},
  {17,25,21,19,17,17,17},{17,17,17,17,17,17,14},{31,4,4,4,4,4,4},
  {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
  {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
  {14,17,17,14,17,17,14},{14,17,17,15,1,1,14},{17,17,10,4,10,17,17},{14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{31,1,2,4,8,16,31},{14,17,17,17,21,18,13},
  {1,1,1,1,17,17,14},{0,0,0,31,0,0,0},{0,0,0,0,0,0,31},{10,31,10,31,10,0,0},
 {0,4,0,0,4,0,0},{0,4,4,31,4,4,0},{1,2,4,8,16,0,0},{0,0,0,0,0,6,6},
 {0,0,0,0,0,0,0},{2,4,8,16,8,4,2},{8,4,2,1,2,4,8}};
 uint32_t maxchars=(x<240u&&y+7u*etch_label_scale<=240u)?(240u-x)/(8u*etch_label_scale):0u;
 static const char lower[]="aefiknoprstu";
 static const uint8_t lowercase[][7]={
  {0,0,14,1,15,17,15}, /* a */
  {6,9,8,28,8,8,8},    /* e? selected via lookup below */
  {0,0,14,17,31,16,14},/* e */
  {2,4,14,4,4,4,4},    /* f (adjusted below) */
  {4,0,12,4,4,4,14},   /* i */
  {16,16,18,20,24,20,18}, /* k */
  {0,0,30,17,17,17,17}, /* n */
  {0,0,14,17,17,17,14}, /* o */
  {0,0,30,17,30,16,16}, /* p */
  {0,0,23,24,16,16,16}, /* r */
  {0,0,15,16,14,1,30}, /* s */
  {4,4,31,4,4,5,2},    /* t */
  {0,0,17,17,17,19,13} /* u */
 };
 for(uint32_t c=0;c<maxchars&&text[c];c++){
  const uint8_t *bits=0;
  for(uint32_t g=0;g<sizeof(names)-1u;g++)if(text[c]==names[g]){bits=glyph[g];break;}
  /* Fallback for lower-case branding text. */
  if(!bits){
   for(uint32_t g=0;g<sizeof(lower)-1u;g++)if(text[c]==lower[g]){
    static const uint8_t indexes[]={0,2,3,4,5,6,7,8,9,10,11,12};
    bits=lowercase[indexes[g]];break;
   }
  }
  if(!bits)continue;
  for(uint32_t row=0;row<7;row++)for(uint32_t col=0;col<5;col++)if(bits[row]&(16u>>col))
   lcd_fill(x+c*8u*etch_label_scale+col*etch_label_scale,y+row*etch_label_scale,
            etch_label_scale,etch_label_scale,color);
 }
}
static void label_large(const char *text,uint32_t x,uint32_t y,uint16_t color){
 etch_label_scale=2u;label(text,x,y,color);etch_label_scale=1u;
}
/* Boot branding is intentionally shown before the audio IRQ begins.
 * No waiting in the realtime audio thread or application main loop. */
/* Minimal centered splash: nothing else on screen. */
static void etch_boot_splash(void){
 lcd_fill(0,0,240,240,0x0000);
 label_large("SIKKENWORKS",32,95,0xffff);
 label("Born out of passion.",40,132,0xffff);
 lcd_sync();fm1_wdt_feed();fm1_delay_ms(480u);fm1_wdt_feed();
}

#include "native_project_codec.h"
#include "native_octave_mode.h"
#include "native_diagnostics.h"
#include "native_panel_capture.h"
#include "native_seq_ui.h"
#include "native_patch_swap.h"
#include "native_preset_browser.h"

static uint8_t native_page,native_mod_mode,native_mod_ratio,native_filter_type;
static uint8_t native_patch_slot,native_patch_confirm,native_patch_wave_warning,native_seq_warning,native_seq_run;
static uint16_t native_seq_length=16;
static uint16_t native_seq_step;
static uint8_t native_seq_rate=2,native_arp_mode,native_arp_rate=2,native_arp_octaves=1,native_arp_gate_index=2,native_seq_gate=75;
static uint8_t native_seq_swing,native_note_sel,native_lock_sel,native_clock_follow;
static uint8_t native_arp_edit_step;
/* Preview slots are kept separate from active edited waves until double-REC. */
static uint8_t native_morph_a,native_morph_b;
static uint8_t native_browser_slot,native_browser_target,native_morph_focus;
static uint8_t native_wave_load_armed,native_wave_load_page;
static uint8_t native_wave_load_error,native_system_tab;
/* P05: four named projects, independent CRC snapshots. SEL confirms project
 * load/new once; explicit SEL+SAVE double-chord still guards deletes. */
static uint8_t native_project_browser=0,native_project_armed=0,native_project_action=0;
static uint8_t native_project_present=0,native_project_char_index=0;
static char native_project_names[4][13];
static char native_save_name[13]; /* staged, never modifies an occupied slot until confirmed */
static uint16_t last_painted_playhead=0xffffu;
/* Set after flash has reported ETCH_STORE_OK, not before saving.
 * Hold the banner until its first successful draw and remove it once on expiry:
 * do not force a heavy LCD refresh every 100 ms while it is visible. */
static uint32_t native_saved_until;
static uint8_t native_save_notice_pending;
static uint8_t native_saved_wave;
static uint8_t native_save_missing_name,native_save_attempted;
static uint8_t native_save_origin_draw,native_save_overwrite;
static uint8_t native_save_preset_mode,native_preset_save_slot;
static uint32_t native_home_tap_at;
static struct etch_home_gesture native_home_gesture;
static uint8_t native_presets_cursor;

static struct etch_diag_panel native_panel_diag;
static uint8_t native_panel_capture;
static uint8_t native_hold_pitch=60,native_guide_note=60,native_edit_feedback;
static uint16_t native_hold_source=0xffffu;
static uint8_t native_seq_clear_armed;static uint16_t native_seq_clear_step;
static uint8_t native_bake_scope,native_bake_confirm;
static struct etch_harmony_settings native_harmony;
static uint8_t native_guide_performance; /* GLO preview never edits SEQ notes */
/* Two independent ISR-visible pattern banks. The main loop only edits the
 * inactive bank during a patch switch; old voices and FX states are preserved. */
static struct etch_patch_swap native_patch_swap __attribute__((section(".pool")));
#define native_pattern (*etch_patch_active(&native_patch_swap))
static uint8_t native_tempo=120,native_seqnotes[64];
/* Live record is session-only. Pattern note data itself persists through the
 * existing protected project/patch serializer. */
static uint8_t native_motion_recording,native_motion_playing;
static uint32_t native_motion_last_record_ms;
static uint8_t native_seq_recording;
static uint8_t native_seq_rec_touched[ETCH_SEQ_STEPS];
/* Captured trigger instances for the simulator's hold-key + SELECT extension.
 * The held note remains attached to its origin step during cursor motion. */
static uint32_t native_seq_entry_mask;
static uint16_t native_seq_entry_step[27];
static uint8_t native_seq_entry_pitch[27];

/* Large project archive lives in the dedicated non-realtime pool. */
static struct etch_project_settings native_project_cfg __attribute__((section(".pool")));
static uint16_t native_riff_seeds[ETCH_PATCH_SLOTS]; /* .bss; PJT21 per-preset riffs */
static uint8_t native_mono_mode;
static struct etch_octave_chord native_octave_chord;
static uint8_t native_harmonic=2; /* K3 in DRAW, 2..16 */
static uint8_t native_gen_mutate=20u;
static uint8_t native_tool=1,native_name_pos,native_name_kind,native_fx_bake_confirm,native_fx_warning;static int16_t native_tool_span=16;
static uint16_t native_chorus,native_delay,native_reverb;
static struct etch_fx_settings native_fx_settings;
static uint8_t native_pre_insert_slot;
static uint16_t native_env_ms[3]={10,200,250},native_sustain=26000;
static uint16_t native_cutoff=8192,native_resonance,native_drive;
static uint32_t native_free_millihz=1000,native_init_held;
/* A destructive storage claim is never automatic. It is permitted only
 * after the UI shows the loss-of-foreign-data warning and the user
 * deliberately holds FX+SAVE for five full seconds on SYSTEM/STORAGE. */
static uint8_t native_storage_setup_armed;
static struct etch_flash_probe native_flash_probe;

static uint8_t native_cache_pattern(uint8_t slot);
static void native_patch_capture(struct etch_native_patch *p);
static void native_preset_snapshot(uint8_t slot);
#include "native_seq_cursor.h"
#include "wave_store_target.c"
#if ETCH_NATIVE_OTA
#include "ota.c"
#include "ota_target.c"
#else
/* No app updater or OTA flash-writing routines in read-only builds. */
static const uint8_t native_updating=0;
#endif
/* A/B/C selection always loads the waveform into the actual audible oscillator.
 * Encoder detents skip blank slots rather than misleadingly changing a number. */
static void native_morph_load_wave(uint8_t wave,int32_t steps){
 if(wave>2u||!steps)return;
 uint8_t prev=native_library.selection[wave];
 uint8_t selected=prev;
 int8_t dir=steps>0?1:-1;
 int32_t count=steps>0?steps:-steps;
 if(count>64)count=64;
 for(int32_t i=0;i<count;i++)selected=etch_ui_wave_next(native_library.valid,selected,dir);
 if(selected==prev||!etch_ui_wave_valid(native_library.valid,selected))return;
 uint8_t old_wave=native_editor.wave,old_slot=native_slot;
 native_editor.wave=wave;native_slot=selected;native_library_load();
 native_editor.wave=old_wave;native_slot=old_slot;
 if(wave==0u)native_morph_a=selected;else if(wave==1u)native_morph_b=selected;
 native_wave_load_armed=0;native_wave_load_error=0;
}
static void native_project_restore(void);
static void native_restore_project_bpm(void);
static void native_patch_load(uint8_t pattern_prepared);
static void native_project_browser_refresh(void){
#if ETCH_P05_MULTI
 uint8_t p=0;char names[4][12]={{0}};
 int rc=etch_store_projects(&native_store_io,&p,names);
 if(rc!=ETCH_STORE_OK)return;
 native_project_present=p;
 if(rc==ETCH_STORE_OK){for(uint32_t i=0;i<4;i++)for(uint32_t j=0;j<12;j++)native_project_names[i][j]=names[i][j];}
#else
 native_project_present=0;
#endif
}
static void native_project_browser_save(uint8_t muted){
#if ETCH_P05_MULTI
 uint8_t old=etch_store_project_selected();
 etch_store_project_select(native_project_browser);
 char title[13]={'P','R','O','J','E','C','T',' ','1',0};
 title[8]=(char)('1'+native_project_browser);
 (void)title;
 etch_store_project_name(native_save_name);
 /* A PROJECT snapshot also remembers the currently edited sound, including
  * unsaved waveform edits. Never overwrite an external waveform slot. */
 if(native_patch_slot<ETCH_PATCH_SLOTS){
  struct etch_native_patch *current=&native_library.patches[native_patch_slot];
  native_patch_capture(current);
  if(!current->name[0]){
   const char *default_name="PATCH";
   for(uint8_t i=0;i<5;i++)current->name[i]=default_name[i];
  }
  native_preset_snapshot(native_patch_slot);
  native_library.reserved=native_patch_slot+1u; /* restore this sound after cold boot */
 }
 native_library_commit(muted);
 if(native_store_status!=ETCH_STORE_OK)etch_store_project_select(old);
 native_project_browser_refresh();
#else
 (void)muted;
#endif
}
static void native_project_browser_delete(uint8_t muted){
#if ETCH_P05_MULTI
 if(native_project_present & (1u<<native_project_browser)){
  /* Never erase the project still in RAM. Open another project first;
   * otherwise the next SAVE could silently recreate the deleted slot. */
  if(native_project_browser==etch_store_project_selected()){
   native_store_status=ETCH_STORE_INCOMPATIBLE;return;
  }
  uint8_t tripped=native_tripped;native_pause();
  native_store_status=etch_store_delete_project_slot(&native_store_io,native_project_browser,ETCH_STORE_ERASE_CONFIRM);
  if(!muted&&!tripped){fm1_irq_off();native_start();fm1_irq_enable_all();}
  native_project_browser_refresh();
 }
#else
 (void)muted;
#endif
}
static void native_project_browser_load(uint8_t muted){
#if ETCH_P05_MULTI
 if(!(native_project_present & (1u<<native_project_browser)))return;
 uint8_t tripped=native_tripped;native_pause();
 int rc=etch_store_load_project_slot(&native_store_io,native_project_browser,
                                    &native_library,&native_pattern,&native_project_cfg);
 native_store_status=rc;
 if(rc==ETCH_STORE_OK){
  native_patch_slot=0;
  native_project_restore();
  for(uint32_t w=0;w<3;w++)for(uint32_t i=0;i<128;i++)native_editor.points[w][i]=native_library.active[w][i];
  native_editor.dirty=1;++native_editor.revision;
  native_patch_slot=(native_library.reserved>=1&&native_library.reserved<=ETCH_PATCH_SLOTS)
     ?native_library.reserved-1:0;
  native_patch_load(0);
  native_restore_project_bpm();
  native_compile_edit();
 }
 if(!muted&&!tripped){fm1_irq_off();native_start();fm1_irq_enable_all();}
#else
 (void)muted;
#endif
}

/* Restoring a project must not discard its stored BPM when the selected
 * patch has a different (older) tempo. Live patch changes still load the
 * selected patch's own BPM through native_patch_load(). */
#include "native_tempo_restore.h"
#include "native_preset_defaults.h"
static void native_restore_project_bpm(void){
 native_tempo=etch_saved_project_bpm(native_project_cfg.tempo,native_tempo);
}
static void native_project_restore(void){
 if(native_store_status!=ETCH_STORE_OK)return;
 native_seq_length=native_project_cfg.seq_length;
 native_seq_rate=native_project_cfg.seq_rate;
 native_seq_gate=native_project_cfg.seq_gate;
 native_seq_swing=native_project_cfg.seq_swing;
 native_clock_follow=native_project_cfg.clock_follow;
 native_tempo=native_project_cfg.tempo;
 native_motion_recording=0;native_motion_playing=0;
 native_harmony=native_project_cfg.harmony;
 native_mono_mode=(uint8_t)(native_project_cfg.mono_mode!=0);
 if(native_project_cfg.source_version>=3)native_fx_settings=native_project_cfg.global_fx;
 for(uint32_t i=0;i<3;i++)native_editor.fx[i]=native_project_cfg.wave_fx[i];
}

static int32_t ui_bound(int32_t v,int32_t lo,int32_t hi){return v<lo?lo:v>hi?hi:v;}
/* The native FM1 keyboard LEDs are physically addressed through the input
 * matrix: 14..40 are the 27 musical keys. Mutate only UI LED planes outside
 * audio processing. GRID maps the first sixteen keys to sixteen steps;
 * NOTES maps all twenty-seven keys to the pitches in the selected chord. */
static void native_seq_led_dim(uint32_t id,uint8_t on){
 for(uint32_t col=0;col<FM1_NCOL;col++)for(uint32_t row=1;row<5;row++)
  if(FM1_KEYMAP[row][col]==(int8_t)id){
   if(on)fm1_led_dim[col]|=(uint8_t)(1u<<row);
   else fm1_led_dim[col]&=(uint8_t)~(1u<<row);
  }
}
/* Mid-bright scale keys remain visible while a played key uses full
 * brightness. Reuse the native LED matrix only, with no audio/MIDI events. */
static void native_seq_led_mid(uint32_t id,uint8_t on){
 for(uint32_t col=0;col<FM1_NCOL;col++)for(uint32_t row=1;row<5;row++)
  if(FM1_KEYMAP[row][col]==(int8_t)id){
   if(on)fm1_led_mid[col]|=(uint8_t)(1u<<row);
   else fm1_led_mid[col]&=(uint8_t)~(1u<<row);
  }
}
static void native_seq_led_refresh(void){
 const uint16_t step_led=etch_seq_actual_step(native_seq_run,native_audio.perf_last_seq,
    native_audio.perf_played,native_audio.perf_current_step,native_seq_length,native_seq_step);
 struct etch_seq_key_view notes={0};struct etch_seq_grid_view grid={0};
 uint32_t solid=0,dim=0,mid=0;
 if(native_page==ETCH_UI_SYSTEM&&native_system_tab==3){
  /* All keys and their LEDs can be exercised without sounding notes. One
   * LED is chased automatically; pressed keys are also lit immediately. */
  solid=fm1_in.notes&ETCH_DIAG_KEYS_MASK;
  if(native_panel_capture)solid|=1u<<((etch_ms/160u)%27u);
 }else if(native_page==18){
  if(!etch_seq_grid_view(&native_pattern,native_seq_length,step_led,&grid)){
   solid=grid.selected;dim=(uint32_t)(grid.triggers|grid.holds|grid.locks)&~solid;
  }
 }else if(native_page==6||native_page==10||native_page==20){
  if(!etch_seq_key_view(&native_pattern,step_led,native_controls.octave,native_note_sel,&notes)){
   solid=notes.selected;dim=(notes.triggers|notes.holds)&~solid;
  }
 }
 /* Pressed keys must ALWAYS light up, regardless of page. Also show the
  * OCT buttons while held; their current nonzero octave has a dim indicator.
  * Keep this in the control loop, never the realtime audio callback. */
 /* In LOCK mode also light the snapped-to note that the audio engine
  * actually plays. Keep the originally held LED on as tactile feedback.
  * GRID, HOLD editing and diagnostics own their LEDs and are excluded. */
 if(!native_panel_capture&&!(native_page==ETCH_UI_SYSTEM&&native_system_tab==3)&&
    native_page!=18&&native_page!=20)
  solid|=etch_led_keylock_targets(fm1_in.notes,native_controls.octave,&native_harmony);
 /* GUIDE highlights all playable scale pitches, not only held keys.
  * Preserve GRID/NOTES/HOLD and diagnostic LED ownership. Existing LOCK
  * indicators still follow the audible quantized note. */
 if(native_harmony.keylock==1u&&!native_panel_capture&&
    !(native_page==ETCH_UI_SYSTEM&&native_system_tab==3)&&
    native_page!=ETCH_UI_GRID&&native_page!=6&&
    native_page!=10&&native_page!=20)
  mid=etch_led_scale_guide_targets(native_controls.octave,&native_harmony);
 solid|=fm1_in.notes&0x07ffffffu;
 dim&=~solid;
 mid&=~solid;
 fm1_irq_off();
 for(uint32_t k=0;k<27;k++){
  uint32_t bit=1u<<k;
  fm1_led_key(14u+k,(solid&bit)!=0);
  native_seq_led_dim(14u+k,(dim&bit)!=0);
  native_seq_led_mid(14u+k,(mid&bit)!=0);
 }
 for(uint8_t b=2u;b<14u;b++)fm1_led_key(b,etch_led_function(b));
 fm1_led_key(0u,etch_led_octave(0u,native_controls.octave,fm1_in.buttons));
 fm1_led_key(1u,etch_led_octave(1u,native_controls.octave,fm1_in.buttons));
 native_seq_led_dim(0u,0);
 native_seq_led_dim(1u,0);
 fm1_irq_enable_all();
}
static uint32_t envelope_step(uint32_t ms){if(!ms)return 0x7fff0000u;uint32_t samples=ms*3000u/68u;return samples?0x7fff0000u/samples:0x7fff0000u;}
static void native_parameter_publish(void){
 native_controls.env_enabled=1;native_controls.attack_step=envelope_step(native_env_ms[0]);native_controls.decay_step=envelope_step(native_env_ms[1]);native_controls.release_step=envelope_step(native_env_ms[2]);native_controls.sustain_level=native_sustain;
 native_controls.filter_type=native_filter_type;native_controls.cutoff=native_cutoff;native_controls.resonance=native_resonance;native_controls.drive=native_drive;
 native_controls.mod_mode=native_mod_mode;native_controls.mod_ratio=native_mod_ratio;if(native_mod_mode==2){
  static const uint16_t units[12]={1,2,4,8,16,32,64,128,256,512,1024,2048};
  native_controls.mod_inc=(uint32_t)(((uint64_t)4294967296ull*native_tempo*64u)/(44100u*60u*units[native_mod_ratio]));
 }else native_controls.mod_inc=(uint32_t)(((uint64_t)native_free_millihz*6380099u)>>16);
 native_controls.chorus=native_chorus;native_controls.delay=native_delay;native_controls.reverb=native_reverb;native_controls.fx_settings=native_fx_settings;
 native_controls.arp_mode=native_arp_mode;native_controls.arp_rate=native_arp_rate;native_controls.arp_octaves=native_arp_octaves;
 native_controls.arp_gate=(uint8_t)(25u+25u*native_arp_gate_index);
 native_controls.arp_program=&native_project_cfg.arp_programs[native_patch_slot];
 native_controls.riff_seed=native_riff_seeds[native_patch_slot];
 native_controls.preview_wave=native_draw?(uint8_t)(native_editor.wave+1u):
   native_page==ETCH_UI_PREFX_A?1u:native_page==ETCH_UI_PREFX_B?2u:0u;
 native_controls.seq_run=native_seq_run;native_controls.seq_length=native_seq_length;native_controls.seq_rate=native_seq_rate;native_controls.seq_gate=native_seq_gate;native_controls.tempo=native_tempo;
 native_controls.pattern=&native_pattern;native_controls.seq_swing=native_seq_swing;
 native_controls.harmony=native_harmony;native_controls.clock_follow=native_clock_follow;
 native_controls.mono_mode=native_mono_mode;
 native_controls.motion=&native_project_cfg.motion;native_controls.motion_play=native_motion_playing;
 native_controls.evolution=native_project_cfg.evolution;
 for(uint32_t i=0;i<64;i++)native_controls.seq_notes[i]=native_seqnotes[i];
}
static char *native_selected_name(void){return native_save_name;}
static void native_save_name_start(const char *src){
 uint32_t i=0;for(;i<12;i++){if(!src||!src[i])break;native_save_name[i]=src[i];}
 for(;i<12;i++)native_save_name[i]=' ';
 native_save_name[12]=0;native_name_pos=0;native_save_overwrite=0;
}
static void native_save_name_copy(char *dst){
 for(uint32_t i=0;i<12;i++)dst[i]=native_save_name[i];
}
static void native_wave_save_start(void){
 native_name_kind=0;native_save_origin_draw=1;
 native_slot=native_library.selection[native_editor.wave];
 uint8_t exists=(uint8_t)((native_library.valid[native_slot>>5]>>(native_slot&31))&1u);
 native_save_name_start(exists?native_library.wave_names[native_slot]:0);
 native_save_missing_name=0u;native_save_attempted=0u;
 native_page=ETCH_UI_NAME;
}
static void native_project_save_start(void){
 native_save_preset_mode=0;
 native_project_browser=etch_store_project_selected();
 native_project_browser_refresh();
 native_save_name_start((native_project_present&(1u<<native_project_browser))?native_project_names[native_project_browser]:0);
 native_save_missing_name=0u;native_save_attempted=0u;
 native_draw=0;native_page=ETCH_UI_PROJECT_SAVE;
}
static void native_preset_save_start(void){
 native_save_preset_mode=1;
 native_preset_save_slot=native_patch_slot;
 native_save_name_start(native_library.patches[native_preset_save_slot].valid?
                        native_library.patches[native_preset_save_slot].name:0);
 native_save_missing_name=0;native_save_attempted=0;
 native_draw=0;native_page=ETCH_UI_PROJECT_SAVE;
}
static void native_new_project(void){
 /* Do not clear or reseed any shared saved wave! Only project-local material.
  * If all four slots are occupied, keep the currently loaded project intact. */
 native_project_browser_refresh();
 if((native_project_present&15u)==15u){
  native_store_status=ETCH_STORE_FULL;
  native_project_browser=4u;
  return;
 }
 for(uint8_t slot=0;slot<4u;slot++)if(!(native_project_present&(1u<<slot))){
  etch_store_project_select(slot);break;
 }
 etch_pattern_clear(&native_pattern);
 native_seq_run=0;native_seq_recording=0;native_seq_step=0;native_seq_length=16;
 native_seq_rate=2;native_seq_gate=75;native_seq_swing=0;
 native_clock_follow=0;native_tempo=120;native_mono_mode=0;native_project_cfg.mono_mode=0;
 for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++){
  native_library.patches[i].valid=0;
  native_project_cfg.pattern_rate[i]=0;
  native_project_cfg.pattern_gate[i]=0;
  native_project_cfg.pattern_swing[i]=0;
  native_project_cfg.arp_programs[i].length=0;
  native_riff_seeds[i]=0; /* Brand-new projects never reuse another project's generated riffs. */
 }
 native_project_cfg.patterns.used=0;
 for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)native_project_cfg.patterns.lengths[i]=0;
 native_project_cfg.patch_fx_valid=0;
 native_project_cfg.preset_valid=0; /* never carry snapshots from another project */
 etch_motion_clear(&native_project_cfg.motion);native_motion_recording=0;native_motion_playing=0;
 native_project_cfg.evolution=(struct etch_evolution_settings){0,30,0,0};
 native_patch_slot=0;native_library.reserved=0;
 native_controls.morph=8192;native_controls.depth=8192;
 native_mod_mode=0;native_mod_ratio=3;
 native_page=ETCH_UI_PLAY;native_draw=0;
}

static const char name_chars[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_#";
static void native_name_turn(int32_t delta){
 if(!delta)return;
 char *p=native_selected_name();uint32_t i=0;
 while(i+1<sizeof(name_chars)-1&&name_chars[i]!=(p[native_name_pos]?p[native_name_pos]:' '))++i;
 int32_t n=(int32_t)i+delta;const int32_t count=(int32_t)sizeof(name_chars)-1;
 n%=count;if(n<0)n+=count;p[native_name_pos]=name_chars[n];
}
static void native_patch_capture(struct etch_native_patch *p){
 p->valid=1;p->slot_a=native_library.selection[0];p->slot_b=native_library.selection[1];p->slot_c=native_library.selection[2];
 p->mod_mode=native_mod_mode;p->mod_target=native_controls.mod_target;p->mod_ratio=native_mod_ratio;p->filter_type=native_filter_type;
 p->arp_mode=native_arp_mode;p->seq_length=native_seq_length==256?0:(uint8_t)native_seq_length;p->tempo=native_tempo;
 p->flags=etch_arp_pack_flags(native_arp_rate,native_arp_octaves,native_arp_gate_index);
 p->morph=native_controls.morph;p->depth=native_controls.depth;
 p->attack_ms=native_env_ms[0];p->decay_ms=native_env_ms[1];p->release_ms=native_env_ms[2];p->sustain=native_sustain;
 p->cutoff=native_cutoff;p->resonance=native_resonance;p->drive=native_drive;p->chorus=native_chorus;p->delay=native_delay;p->reverb=native_reverb;
}
static void native_preset_snapshot(uint8_t slot){
 if(slot>=ETCH_PATCH_SLOTS)return;
 native_project_cfg.preset_valid|=1u<<slot;
 native_project_cfg.preset_fx[slot]=native_fx_settings;
 for(uint32_t w=0;w<3;w++)for(uint32_t i=0;i<128;i++)
  native_project_cfg.preset_wave[slot][w][i]=native_editor.points[w][i];
 native_project_cfg.patch_fx_valid|=1u<<slot;
 for(uint32_t w=0;w<3;w++)native_project_cfg.patch_fx[slot][w]=native_editor.fx[w];
}
static uint8_t native_cache_pattern(uint8_t slot){
 int rc=etch_pattern_bank_capture(&native_project_cfg.patterns,slot,&native_pattern);
 if(rc){native_seq_warning=1;return 0;}
 native_project_cfg.pattern_rate[slot]=native_seq_rate;
 native_project_cfg.pattern_gate[slot]=native_seq_gate;
 native_project_cfg.pattern_swing[slot]=native_seq_swing;
 return 1;
}
static void native_patch_load(uint8_t pattern_prepared){
 struct etch_native_patch *p=&native_library.patches[native_patch_slot];if(!p->valid)return;
 if(native_project_cfg.source_version>=4){
  /* Boot loads the active buffer. Live switches use the already validated
   * inactive buffer and never overwrite an ISR-visible pattern. */
  if(!pattern_prepared && etch_pattern_bank_fetch(&native_project_cfg.patterns,native_patch_slot,&native_pattern)<0){native_seq_warning=1;return;}
  native_seq_rate=native_project_cfg.pattern_rate[native_patch_slot];
  native_seq_gate=native_project_cfg.pattern_gate[native_patch_slot]?native_project_cfg.pattern_gate[native_patch_slot]:75;
  native_seq_swing=native_project_cfg.pattern_swing[native_patch_slot];
 }
 uint8_t slots[3]={p->slot_a,p->slot_b,p->slot_c};
 for(uint32_t w=0;w<3;w++)if(slots[w]<64&&(native_library.valid[slots[w]>>5]&(1u<<(slots[w]&31u)))){
  for(uint32_t i=0;i<128;i++)native_editor.points[w][i]=native_library.slot[slots[w]][i];
  native_library.selection[w]=slots[w];
  native_editor.fx[w]=native_project_cfg.patch_fx_valid&(1u<<native_patch_slot)?native_project_cfg.patch_fx[native_patch_slot][w]:native_project_cfg.slot_fx[slots[w]];
 }
 if(native_project_cfg.preset_valid&(1u<<native_patch_slot)){
  for(uint32_t w=0;w<3;w++){
   for(uint32_t i=0;i<128;i++)
    native_editor.points[w][i]=native_project_cfg.preset_wave[native_patch_slot][w][i];
   if(native_project_cfg.patch_fx_valid&(1u<<native_patch_slot))
    native_editor.fx[w]=native_project_cfg.patch_fx[native_patch_slot][w];
  }
  native_fx_settings=native_project_cfg.preset_fx[native_patch_slot];
 }else{
  /* PJT12 and older: the preset still references its original wave slots.
   * Global FX fall back to the legacy project's shared FX. */
  native_fx_settings=native_project_cfg.global_fx;
  /* Legacy/factory preset without an independent FX snapshot: do not
   * inherit a Wave C sculpting effect or FX modulation from the project.
   * User-saved PJT17 snapshots above retain their explicit settings. */
  etch_preset_legacy_c_fx_off(&native_editor.fx[2],&native_fx_settings);
 }
 native_editor.dirty=1;++native_editor.revision;
 native_mod_mode=p->mod_mode>2?0:p->mod_mode;native_mod_ratio=p->mod_ratio<12?p->mod_ratio:0;
 fm1_irq_off();native_controls.mod_target=p->mod_target&1u;fm1_irq_enable_all();native_filter_type=p->filter_type<4?p->filter_type:0;
 native_arp_mode=p->arp_mode<=7?p->arp_mode:0;
 etch_arp_unpack_flags(p->flags,&native_arp_rate,&native_arp_octaves,&native_arp_gate_index);
 native_seq_length=p->seq_length==0?256:(p->seq_length==128||p->seq_length==64||p->seq_length==32||p->seq_length==8?p->seq_length:16);
 native_tempo=etch_saved_project_bpm(native_project_cfg.tempo,p->tempo);
 native_seq_warning=0;
 fm1_irq_off();native_controls.morph=p->morph>32767?32767:p->morph;native_controls.depth=p->depth>32767?32767:p->depth;fm1_irq_enable_all();
 native_env_ms[0]=p->attack_ms;native_env_ms[1]=p->decay_ms;native_env_ms[2]=p->release_ms;native_sustain=p->sustain;
 native_cutoff=p->cutoff;native_resonance=p->resonance;native_drive=p->drive;
 native_chorus=p->chorus;native_delay=p->delay;native_reverb=p->reverb;
 if(!pattern_prepared){native_seq_run=0;native_seq_recording=0;}native_library.reserved=native_patch_slot+1;
 native_morph_a=native_library.selection[0];native_morph_b=native_library.selection[1];
 /* Atomic ISR handoff: all new parameters and the output crossfade start
  * at the same publication boundary. Cold boot has no live preset ramp. */
 fm1_irq_off();native_parameter_publish();if(pattern_prepared)native_preset_ramp_requested=1u;fm1_irq_enable_all();
}
static void native_patch_save(uint8_t muted){
 /* Native PRESET SAVE: stage complete 3-wave sound, per-wave processing,
  * morph, envelopes, modulation, global FX, and ARP/pattern in PJT13.
  * Never modify an existing shared wave slot or require a separate WAVE SAVE.
  * A failed/full flash commit retains the previously verified project bank. */
 uint8_t source=native_patch_slot,target=native_preset_save_slot;
 if(target>=ETCH_PATCH_SLOTS){native_store_status=ETCH_STORE_VERIFY;return;}
 native_patch_slot=target;
 if(!native_cache_pattern(target)){
  native_patch_slot=source;native_store_status=ETCH_STORE_FULL;return;
 }
 struct etch_native_patch *p=&native_library.patches[target];
 native_patch_capture(p);
 native_save_name_copy(p->name);
 native_preset_snapshot(target);
 native_library.reserved=target+1u;
 native_library_commit(muted);
 if(native_store_status!=ETCH_STORE_OK){
  /* No verified save happened; the source patch remains active in RAM. */
  native_patch_slot=source;
  native_library.reserved=source+1u;
 }else{
  native_patch_confirm=0;native_patch_wave_warning=0;
 }
}

/* Flash-free patch transition: stage the next pattern outside ALNK0, then
 * publish a new stable pointer. Never stop/reinitialise DMA or FX buffers.
 * Wave tables are rebuilt independently and use the existing 256-sample
 * crossfade; morph/depth also have the existing per-sample slew. */
static void native_patch_change(uint8_t next){
 if(next==native_patch_slot||next>=ETCH_PATCH_SLOTS||!native_library.patches[next].valid)return;
 if(!native_cache_pattern(native_patch_slot))return;
 if(native_project_cfg.source_version>=4){
  if(etch_patch_prepare(&native_patch_swap,&native_project_cfg.patterns,next)<0){native_seq_warning=1;return;}
 }else etch_pattern_clear(etch_patch_staging(&native_patch_swap));
 /* Keep transport running and avoid an unintended NOTE-OFF/Start cycle. */
 native_patch_slot=next;native_patch_confirm=0;
 /* IRQ masks the publication itself; the old buffer remains intact. */
 fm1_irq_off();etch_patch_commit(&native_patch_swap);fm1_irq_enable_all();
 native_patch_load(1);
}
static void number(uint32_t v,uint32_t x,uint32_t y){char s[11];uint32_t i=10;s[i]=0;do{s[--i]=(char)('0'+v%10);v/=10;}while(v&&i);label(s+i,x,y,0xffff);}
static void parameters(void){
 lcd_fill(0,196,240,40,0x0000);
 const char *names[4];uint32_t values[4];
 if(native_page==ETCH_UI_ENV){names[0]="A MS";names[1]="D MS";names[2]="S";names[3]="R MS";values[0]=native_env_ms[0];values[1]=native_env_ms[1];values[2]=native_sustain*100u/32767u;values[3]=native_env_ms[2];}
 else if(native_page==ETCH_UI_PREFX_A||native_page==ETCH_UI_PREFX_B){
  uint8_t wave=native_page==ETCH_UI_PREFX_B;
  names[0]="SLOT";names[1]="TYPE";names[2]="DEPTH";names[3]="MIX";
  values[0]=native_pre_insert_slot+1u;
  values[1]=etch_pre_type(&native_fx_settings,wave,native_pre_insert_slot);
  values[2]=etch_pre_depth(&native_fx_settings,wave,native_pre_insert_slot)*100u/32767u;
  values[3]=etch_pre_mix(&native_fx_settings,wave,native_pre_insert_slot)*100u/32767u;
 }
 else if(native_page==ETCH_UI_C_FX){
  names[0]="TARGET";names[1]="DEPTH";names[2]="RATE";names[3]="C WAVE";
  values[0]=etch_c_fx_target(&native_fx_settings);
  values[1]=etch_c_fx_depth(&native_fx_settings)*100u/32767u;
  values[2]=native_mod_mode==1?native_free_millihz/1000u:native_mod_ratio+1u;
  values[3]=etch_ui_wave_ordinal(native_library.valid,native_library.selection[2]);
 }
 else if(native_page==2){names[0]="WAVE C";names[1]="DEPTH";names[2]="RATE";names[3]="TARGET";values[0]=etch_ui_wave_ordinal(native_library.valid,native_library.selection[2]);values[1]=native_controls.depth*100u/32767u;values[2]=native_mod_mode==1?native_free_millihz/1000u:native_mod_ratio+1u;values[3]=native_controls.mod_target;}
 else if(native_page==3){names[0]="TYPE";names[1]="CUTOFF";names[2]="RESO";names[3]="DRIVE";values[0]=native_filter_type;values[1]=native_cutoff;values[2]=native_resonance*100u/32767u;values[3]=native_drive*100u/32767u;}
 else if(native_page==ETCH_UI_FX_DRIVE){
  names[0]="DRIVE";names[1]="TYPE";names[2]="CUTOFF";names[3]="RESO";
  values[0]=native_drive*100u/32767u;values[1]=native_filter_type;values[2]=native_cutoff;values[3]=native_resonance*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_CHORUS){
  names[0]="MIX";names[1]="RATE";names[2]="DEPTH";names[3]="ROUTE";
  values[0]=native_chorus*100u/32767u;values[1]=native_fx_settings.chorus_rate*100u/32767u;
  values[2]=native_fx_settings.chorus_depth*100u/32767u;values[3]=native_fx_settings.routing;
 }
 else if(native_page==ETCH_UI_FX_DELAY){
  names[0]="MIX";names[1]="TIME";names[2]="FBACK";names[3]="ROUTE";
  values[0]=native_delay*100u/32767u;values[1]=native_fx_settings.delay_time*100u/32767u;
  values[2]=native_fx_settings.delay_feedback*100u/32767u;values[3]=native_fx_settings.routing;
 }
 else if(native_page==ETCH_UI_FX_RING){
  names[0]="MIX";names[1]="RATE HZ";names[2]="SHAPE";names[3]="BIAS";
  uint32_t rv=native_fx_settings.ring_rate;
  values[0]=native_fx_settings.ring_mix*100u/32767u;
  values[1]=1u+(uint32_t)(((uint64_t)rv*rv*1599u)/(32767u*32767u));
  values[2]=native_fx_settings.ring_shape*100u/32767u;
  values[3]=native_fx_settings.ring_bias*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_EQ){
  names[0]="LOW";names[1]="MID";names[2]="HIGH";names[3]="HPF";
  values[0]=native_fx_settings.eq_low*200u/32767u;
  values[1]=native_fx_settings.eq_mid*200u/32767u;
  values[2]=native_fx_settings.eq_high*200u/32767u;
  values[3]=native_fx_settings.eq_hpf*400u/32767u;
 }
 else if(native_page==ETCH_UI_FX_COMP){
  names[0]="THRESH";values[0]=native_fx_settings.comp_threshold*100u/32767u;
  names[1]="RATIO";values[1]=native_fx_settings.comp_ratio*100u/32767u;
  names[2]="ATTACK";values[2]=native_fx_settings.comp_attack*100u/32767u;
  names[3]="RELEASE";values[3]=native_fx_settings.comp_release*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_TREM){
  names[0]="RATE";values[0]=native_fx_settings.trem_rate*100u/32767u;
  names[1]="DEPTH";values[1]=native_fx_settings.trem_depth*100u/32767u;
  names[2]="SHAPE";values[2]=native_fx_settings.trem_shape*100u/32767u;
  names[3]="MIX";values[3]=native_fx_settings.trem_mix*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_PHASER){
  names[0]="RATE";values[0]=native_fx_settings.phaser_rate*100u/32767u;
  names[1]="DEPTH";values[1]=native_fx_settings.phaser_depth*100u/32767u;
  names[2]="FEED";values[2]=native_fx_settings.phaser_feedback*100u/32767u;
  names[3]="MIX";values[3]=native_fx_settings.phaser_mix*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_FLANGER){
  names[0]="RATE";values[0]=native_fx_settings.flanger_rate*100u/32767u;
  names[1]="DEPTH";values[1]=native_fx_settings.flanger_depth*100u/32767u;
  names[2]="FEED";values[2]=native_fx_settings.flanger_feedback*100u/32767u;
  names[3]="MIX";values[3]=native_fx_settings.flanger_mix*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_DECIM){
  names[0]="BITS";values[0]=native_fx_settings.decim_bits*100u/32767u;
  names[1]="RATE";values[1]=native_fx_settings.decim_rate*100u/32767u;
  names[2]="DRIVE";values[2]=native_fx_settings.decim_drive*100u/32767u;
  names[3]="MIX";values[3]=native_fx_settings.decim_mix*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_FOLDER){
  names[0]="FOLD";values[0]=native_fx_settings.folder_fold*100u/32767u;
  names[1]="SYMM";values[1]=native_fx_settings.folder_symmetry*100u/32767u;
  names[2]="TONE";values[2]=native_fx_settings.folder_tone*100u/32767u;
  names[3]="MIX";values[3]=native_fx_settings.folder_mix*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_TAPE){
  names[0]="WOW";values[0]=native_fx_settings.tape_wow*100u/32767u;
  names[1]="FLUTTER";values[1]=native_fx_settings.tape_flutter*100u/32767u;
  names[2]="AGE";values[2]=native_fx_settings.tape_age*100u/32767u;
  names[3]="MIX";values[3]=native_fx_settings.tape_mix*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_REVERSE){
  names[0]="TIME";values[0]=native_fx_settings.reverse_time*100u/32767u;
  names[1]="FEED";values[1]=native_fx_settings.reverse_feed*100u/32767u;
  names[2]="TONE";values[2]=native_fx_settings.reverse_tone*100u/32767u;
  names[3]="MIX";values[3]=native_fx_settings.reverse_mix*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_VIBRATO){
  names[0]="RATE";values[0]=native_fx_settings.vibrato_rate*100u/32767u;
  names[1]="DEPTH";values[1]=native_fx_settings.vibrato_depth*100u/32767u;
  names[2]="SHAPE";values[2]=native_fx_settings.vibrato_shape*100u/32767u;
  names[3]="MIX";values[3]=native_fx_settings.vibrato_mix*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_ROTARY){
  names[0]="RATE";values[0]=native_fx_settings.rotary_rate*100u/32767u;
  names[1]="DEPTH";values[1]=native_fx_settings.rotary_depth*100u/32767u;
  names[2]="DRIVE";values[2]=native_fx_settings.rotary_drive*100u/32767u;
  names[3]="MIX";values[3]=native_fx_settings.rotary_mix*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_WAH){
  names[0]="SENS";values[0]=native_fx_settings.wah_sens*100u/32767u;
  names[1]="ATTACK";values[1]=native_fx_settings.wah_attack*100u/32767u;
  names[2]="RESO";values[2]=native_fx_settings.wah_reso*100u/32767u;
  names[3]="MIX";values[3]=native_fx_settings.wah_mix*100u/32767u;
 }
 else if(native_page==ETCH_UI_FX_REVERB){
  names[0]="MIX";names[1]="SIZE";names[2]="DECAY";names[3]="ROUTE";
  values[0]=native_reverb*100u/32767u;values[1]=native_fx_settings.reverb_size*100u/32767u;
  values[2]=native_fx_settings.reverb_decay*100u/32767u;values[3]=native_fx_settings.routing;
 }
 else if(native_page==4){names[0]="DRIVE";names[1]="CHORUS";names[2]="DELAY";names[3]="REVERB";values[0]=native_drive*100u/32767u;values[1]=native_chorus*100u/32767u;values[2]=native_delay*100u/32767u;values[3]=native_reverb*100u/32767u;}
 else if(native_page==16){names[0]="C RATE";names[1]="C DEPTH";names[2]="D TIME";names[3]="D FB";
  values[0]=native_fx_settings.chorus_rate*100u/32767u;values[1]=native_fx_settings.chorus_depth*100u/32767u;
  values[2]=native_fx_settings.delay_time*100u/32767u;values[3]=native_fx_settings.delay_feedback*100u/32767u;}
 else if(native_page==17){names[0]="R SIZE";names[1]="R DECAY";names[2]="ROUTE";names[3]="D WET";
  values[0]=native_fx_settings.reverb_size*100u/32767u;values[1]=native_fx_settings.reverb_decay*100u/32767u;
  values[2]=native_fx_settings.routing;values[3]=native_delay*100u/32767u;}
 else if(native_page==5){names[0]="MODE";names[1]="RATE";names[2]="OCT";
  names[3]=native_arp_mode==7u?"NEW RIFF":"BPM";
  values[0]=native_arp_mode;values[1]=native_arp_rate;values[2]=native_arp_octaves;
  values[3]=native_arp_mode==7u?(native_riff_seeds[native_patch_slot]&999u):native_tempo;}
 else if(native_page==23){names[0]="WAVE A";names[1]="WAVE B";names[2]="MORPH";names[3]="C RATE";
  values[0]=native_morph_a+1u;values[1]=native_morph_b+1u;
  values[2]=native_controls.morph*100u/32767u;
  values[3]=native_mod_ratio+1u;}
 else if(native_page==24){names[0]="SLOT";names[1]="TARGET";names[2]="";names[3]="";
  values[0]=native_browser_slot+1;values[1]=native_browser_target+1;values[2]=0;values[3]=0;}
 else if(native_page==22){
  const struct etch_arp_program *a=&native_project_cfg.arp_programs[native_patch_slot];
  names[0]="STEP";names[1]="KEY";names[2]="REST";names[3]="LEN";
  values[0]=native_arp_edit_step+1;values[1]=a->length&&a->steps[native_arp_edit_step]!=255?a->steps[native_arp_edit_step]:0;
  values[2]=a->length&&a->steps[native_arp_edit_step]==255;values[3]=a->length;
 }
 else if(native_page==6){names[0]="LEN";names[1]="RATE";names[2]="SWING";names[3]="GATE";values[0]=native_seq_length;values[1]=native_seq_rate;values[2]=native_seq_swing;values[3]=native_seq_gate;}
 else if(native_page==14){const struct etch_wave_fx_params *f=&native_editor.fx[native_editor.wave];
  names[0]="DRIVE";names[1]="BIT";names[2]="CHORUS";names[3]="DELAY";
  values[0]=f->drive*100u/32767u;values[1]=f->bitcrush*100u/32767u;
  values[2]=f->chorus*100u/32767u;values[3]=f->delay*100u/32767u;}
 else if(native_page==15){const struct etch_wave_fx_params *f=&native_editor.fx[native_editor.wave];
  names[0]="REVERB";names[1]="FOLD";names[2]="SMOOTH";names[3]="MIX";
  values[0]=f->reverb*100u/32767u;values[1]=f->fold*100u/32767u;
  values[2]=f->smooth*100u/32767u;values[3]=f->mix*100u/32767u;}
 else if(native_page==10){
  const struct etch_seq_step *st=&native_pattern.steps[native_seq_step];uint8_t idx=native_note_sel<st->count?native_note_sel:0;
  names[0]="VOICE";names[1]="VEL";names[2]="LENGTH";names[3]="NOTE";
  values[0]=st->count?idx+1:0;values[1]=st->count?st->notes[idx].velocity:0;
  values[2]=st->count?st->notes[idx].length:0;values[3]=st->count?st->notes[idx].note:0;
 }
 else if(native_page==18){names[0]="PAGE";names[1]="STEP";names[2]="TRIG";names[3]="HOLD";
  values[0]=native_seq_step/16+1;values[1]=native_seq_step+1;
  values[2]=native_pattern.steps[native_seq_step].count;values[3]=native_pattern.steps[native_seq_step].hold_count;}
 else if(native_page==20){names[0]="STEP";names[1]="PITCH";names[2]="SRC";names[3]="HOLD";
  values[0]=native_seq_step+1;values[1]=native_hold_pitch;
  values[2]=native_hold_source==0xffffu?0:(native_hold_source+1);
  uint8_t has=0;const struct etch_seq_step *hs=&native_pattern.steps[native_seq_step];
  for(uint8_t i=0;i<hs->hold_count;i++)if(hs->holds[i]==native_hold_pitch)has=1;
  values[3]=has;}
 else if(native_page==21){names[0]="SCOPE";names[1]="STEP";names[2]="CHORD";names[3]="LOCK";
  values[0]=native_bake_scope;values[1]=native_seq_step+1;values[2]=native_harmony.chord;values[3]=native_harmony.keylock;}
 else if(native_page==26){names[0]="KEY";names[1]="SCALE";names[2]="LOCK";names[3]="CHORD";
  values[0]=etch_ui_fifths_index(native_harmony.root)+1;values[1]=native_harmony.mode;values[2]=native_harmony.keylock;values[3]=native_harmony.chord;}
 else if(native_page==19){names[0]="KEY";names[1]="SCALE";names[2]="KEYLOCK";names[3]="PITCH";
  values[0]=native_harmony.root;values[1]=native_harmony.mode;values[2]=native_harmony.keylock;values[3]=native_guide_note;}
 else if(native_page==11){names[0]="KEY";names[1]="MODE";names[2]="KEYLOCK";names[3]="CHORD";
  values[0]=native_harmony.root;values[1]=native_harmony.mode;values[2]=native_harmony.keylock;values[3]=native_harmony.chord;}
 else if(native_page==12){const struct etch_seq_step *st=&native_pattern.steps[native_seq_step];
  names[0]="LOCK";names[1]="VALUE";names[2]="ON";names[3]="STEP";
  values[0]=native_lock_sel;values[1]=st->locks[native_lock_sel];values[2]=(st->lock_mask>>native_lock_sel)&1u;values[3]=native_seq_step+1;}
 else if(native_page==13){names[0]="CLOCK";names[1]="GATE";names[2]="SWING";names[3]="TEMPO";
  values[0]=native_clock_follow;values[1]=native_seq_gate;values[2]=native_seq_swing;values[3]=native_tempo;}
 else if(native_page==25){
  names[0]="TAB";values[0]=native_system_tab+1;
  if(native_system_tab==3){
   names[1]="KEYS";names[2]="BTN";names[3]="K1";
   values[1]=etch_diag_count(native_panel_diag.key_seen);
   values[2]=etch_diag_count(native_panel_diag.button_seen);
   values[3]=native_panel_diag.turns[2];
  }else if(native_system_tab==4){
   names[1]="CPU";names[2]="LATE";names[3]="PEAK";
   values[1]=etch_diag_cpu_permille(native_max_ticks,ETCH_NATIVE_FRAMES*544u)/10u;
   values[2]=native_late;values[3]=native_audio_peak;
  }else{
   names[1]="USB";names[2]="STORE";names[3]="SAFE";
   values[1]=usb.config?1:0;values[2]=native_store_status==ETCH_STORE_OK?1:0;values[3]=native_safe?1:0;
  }
 }
 else if(native_page==27){names[0]="PROJECT";names[1]="POS";names[2]="PRESENT";names[3]="ARM";
  values[0]=native_project_browser+1;values[1]=native_project_char_index+1;
  values[2]=native_project_browser<4u?(native_project_present>>native_project_browser)&1u:0;values[3]=native_project_armed;}
 else if(native_page==ETCH_UI_EVOLUTION){names[0]="DEPTH";names[1]="RATE";names[2]="SHAPE";names[3]="SYNC";
  values[0]=native_project_cfg.evolution.depth;values[1]=native_project_cfg.evolution.rate;
  values[2]=native_project_cfg.evolution.mode;values[3]=native_project_cfg.evolution.sync;}
 else if(native_page==ETCH_UI_MOTION){names[0]="MORPH";names[1]="ETCH";names[2]="CUT";names[3]="SPACE";values[0]=native_controls.morph*100u/32767u;values[1]=native_controls.depth*100u/32767u;values[2]=native_cutoff;values[3]=native_reverb*100u/32767u;}
 else if(native_page==7){names[0]="PATCH";names[1]="MORPH";names[2]="DEPTH";names[3]="SPACE";values[0]=native_patch_slot;values[1]=native_controls.morph*100u/32767u;values[2]=native_controls.depth*100u/32767u;values[3]=native_reverb*100u/32767u;}
 else if(native_page==8||native_page==ETCH_UI_PROJECT_SAVE){names[0]="SLOT";names[1]="POS";names[2]="CHAR";names[3]="DEL";values[0]=(native_page==8?native_slot:native_project_browser)+1u;values[1]=native_name_pos+1u;values[2]=(uint8_t)native_selected_name()[native_name_pos];values[3]=0;}
 else if(native_page==9){
  names[0]="TOOL";names[1]=native_tool==12u?"A/B MIX":"SPAN";
  names[2]=native_tool==12u?"SEED":"CURSOR";
  names[3]=native_tool==12u?"MUTATE":"APPLY";
  values[0]=native_tool;
  values[1]=native_tool==12u?(uint16_t)(native_tool_span+127)*100u/254u:(uint16_t)native_tool_span;
  values[2]=native_editor.cursor;
  values[3]=native_tool==12u?native_gen_mutate:1u;
 }
 else{names[0]="MORPH";names[1]="DEPTH";names[2]="CUTOFF";names[3]="SPACE";values[0]=native_controls.morph*100u/32767u;values[1]=native_controls.depth*100u/32767u;values[2]=native_cutoff;values[3]=native_reverb*100u/32767u;}
 if(native_page==ETCH_UI_PREFX_A||native_page==ETCH_UI_PREFX_B){
  static const char *const types[]={"OFF","RING","DECIM","FOLD","TREM","LP"};
  uint8_t wave=native_page==ETCH_UI_PREFX_B;
  label(types[etch_pre_type(&native_fx_settings,wave,native_pre_insert_slot)],65,184,0xdfe4);
 }
 if(native_page==ETCH_UI_C_FX){
  label(etch_c_fx_name(etch_c_fx_target(&native_fx_settings)),10,184,0xdfe4);
 }
 for(uint32_t i=0;i<4;i++){
  /* Wave Library only uses K1/K2: do not show inactive K3/K4 values. */
  if(native_page==ETCH_UI_LIBRARY && i>=2u)continue;
  label(names[i],4+i*60,198,0x7bef);
  if(i==0 && (native_page==ETCH_UI_GUIDE || native_page==ETCH_UI_HARM || native_page==ETCH_UI_FIFTHS))
   label(etch_note_name(native_harmony.root),4+i*60,214,0xffff);
  else if(i==1 && (native_page==ETCH_UI_GUIDE || native_page==ETCH_UI_HARM || native_page==ETCH_UI_FIFTHS))
   label(etch_mode_short(native_harmony.mode),4+i*60,214,0xffff);
  else if(i==2 && (native_page==ETCH_UI_GUIDE || native_page==ETCH_UI_HARM || native_page==ETCH_UI_FIFTHS))
   label(etch_lock_name(native_harmony.keylock),4+i*60,214,0xffff);
  else if(native_page==ETCH_UI_ARP && i==2 && native_arp_octaves>=5u)
   label(native_arp_octaves==6u?"-2":"-1",4+i*60,214,0xffff);
  else number(values[i],4+i*60,214);
  if(native_page==4||native_page==16||native_page==17||etch_ui_is_fx_editor(native_page)){
   /* Direct numeric feedback AND a short horizontal value meter on FX pages. */
   uint32_t percent=values[i]>100u?100u:values[i];
   lcd_fill(4+i*60,233,(percent*54u)/100u,2,
            i==0?0x67ff:i==1?0xfde8:i==2?0xf95b:0x07e0);
  }
 }
}

static uint8_t first_frame=1;
/* Native, 240x240 recreation of the original simulator's waveform-first
 * PLAY/DRAW/MORPH/MOD/ENV/FILTER/FX/ARP pages (sim/screen.mjs).
 * Secondary editor/diagnostic pages keep their older layout and callbacks.
 * Keep the graph in the RAM DMA canvas; no full-screen clear on note changes. */
static uint8_t web_last_page=255u,web_last_draw=255u,web_last_fault=255u;
static uint32_t web_last_controls=0xffffffffu;
static int16_t web_scope_frame[1024]; /* zero-initialized scope staging */
static uint8_t web_scope_sweep;
static uint8_t web_last_signal=255u;
/* PLAY display-only note release state. Never changes the audio path. */
static struct etch_scope_release_state web_scope_release;
/* Fixed memory visual zoom across frames: scoped to UI, never audio DSP. */
static uint16_t web_scope_zoom_q8=768u;
static uint16_t web_scope_phase_start=0u;
static int32_t web_scope_center_smooth=0;
static uint8_t web_scope_center_ready=0u;
static void web_ui_primary(void){
 uint8_t draw=native_draw && native_page==ETCH_UI_PLAY;
 uint8_t page=native_page;
 uint8_t full=(uint8_t)(first_frame || web_last_page!=page || web_last_draw!=draw);
 const uint16_t BG=0x0000,GRID=0x2124,WHITE=0xf7de,CYAN=0x67ff,AMBER=0xfde8,MAGENTA=0xf95b,LIME=0xdfe4;
 if(full){
  /* Compose header before it becomes visible: no black flash on navigation. */
  etch_scope_begin(0u,42u);
  lcd_fill(0,0,240,42,BG);
  label_large("ETCH",12,9,LIME);
  const char *title=draw?"DRAW":page==ETCH_UI_MOD?"MOD WAVE":
   page==ETCH_UI_FX_TAPE?"TAPE INSTAB.":etch_ui_title(page);
  label(title,88,14,WHITE);
  label(ETCH_UI_VERSION,196,12,0x7bef);
  lcd_fill(12,33,216,1,GRID);
  etch_scope_commit();
 }
 const uint16_t top=42,bottom=154,mid=(top+bottom)/2,extent=51;
 /* Only the graph is erased, inside the double-buffered LCD transfer. */
 etch_scope_begin(top,bottom-top+1);
 lcd_fill(0,top,240,bottom-top+1,BG);
 for(uint32_t i=0;i<=8;i++)lcd_fill(14+i*212/8,top,1,bottom-top+1,GRID);
 for(uint32_t i=0;i<=4;i++)lcd_fill(14,top+i*(bottom-top)/4,212,1,GRID);
 int16_t morphed[128];
 /* Copy the last complete audio frame without pausing the audio DMA IRQ.
  * Published capture stays untouched until the next full 512-sample frame. */
 uint8_t captured=native_scope_ready;
 for(uint32_t i=0;i<1024u;i++)web_scope_frame[i]=native_scope[captured][i];
 /* Releasing every note must finish on the exact centre line even when
  * long reverb/delay feedback keeps the PCM peak above the silence gate.
  * A newly opened page with no held note starts from a flat line. */
 uint8_t held_note=native_scope_held_note;
 if(full && !held_note){web_scope_release.had_held_note=0u;
                         web_scope_release.releasing=0u;}
 uint16_t release_alpha=etch_scope_release_step(&web_scope_release,held_note,etch_ms);
 uint8_t visible_signal=etch_scope_show_audio(native_scope_has_signal);
 uint16_t window_start=etch_ui_scope_trigger_near(web_scope_frame,1024u,512u,web_scope_phase_start);
 if(native_scope_has_signal)web_scope_phase_start=window_start;
 const int16_t *audible=web_scope_frame+window_start;
 uint32_t live_peak=0u;
 int32_t live_offset=0;
 etch_ui_scope_stats(audible,512u,&live_offset,&live_peak);
 /* Avoid per-frame normalization; use post-MASTER peak and knob target
  * only for a slowly-changing LCD zoom estimate. */
 if(visible_signal)web_scope_zoom_q8=etch_ui_scope_master_zoom(
    live_peak,native_controls.gain,web_scope_zoom_q8);
 const int16_t *a=native_editor.points[0],*b=native_editor.points[1];
 if(page==ETCH_UI_MORPH){
  if(etch_ui_wave_valid(native_library.valid,native_morph_a))a=native_library.slot[native_morph_a];
  if(etch_ui_wave_valid(native_library.valid,native_morph_b))b=native_library.slot[native_morph_b];
  etch_ui_wave_morph(a,b,native_controls.morph,morphed);
 }
 const int16_t *visual=draw?native_editor.points[native_editor.wave]:page==ETCH_UI_MORPH?morphed:audible;
 uint16_t visual_count=(!draw&&page!=ETCH_UI_MORPH)?512u:128u;
 int32_t audible_offset=0;
 if(!draw && page!=ETCH_UI_MORPH && visible_signal){
  web_scope_center_smooth=etch_ui_scope_smooth_center(web_scope_center_smooth,
      live_offset,(uint8_t)(!web_scope_center_ready||full));
  web_scope_center_ready=1u;
  audible_offset=web_scope_center_smooth;
 }else if(!visible_signal)web_scope_center_ready=0u;
 /* Restore the larger V1.07 live waveform. A 4-frame smoothed zoom
  * holds across ARP gates, while MASTER still governs visible amplitude.
  * Only LCD values are touched; not DMA/voice/FX/output gain. */
 uint16_t g=(!draw && page!=ETCH_UI_MORPH)?web_scope_zoom_q8:
   etch_ui_wave_display_gain(visual,visual_count);
 if(!draw && page!=ETCH_UI_MORPH)
  g=etch_scope_visual_gain(g,release_alpha,visible_signal);
 uint32_t old=mid,old_a=mid,old_b=mid,old_c=mid;
 const int16_t *c=native_editor.points[2];
 for(uint32_t x=14;x<=226;x++){
  uint32_t j=(x-14u)*127u/212u;
  uint32_t live_j=(x-14u)*511u/212u;
  int16_t v=visual[visual_count==512u?live_j:j];
  if(!draw && page!=ETCH_UI_MORPH)v=etch_ui_scope_centered(v,audible_offset);
  uint32_t y=etch_ui_wave_display_y(v,g,mid,extent,top+1,bottom-1);
  if(page==ETCH_UI_MORPH){
   uint32_t ay=etch_ui_wave_display_y(a[j],g,mid,extent,top+1,bottom-1);
   uint32_t by=etch_ui_wave_display_y(b[j],g,mid,extent,top+1,bottom-1);
   etch_scope_stroke(x,x==14u?ay:old_a,ay,CYAN);etch_scope_stroke(x,x==14u?by:old_b,by,AMBER);
   old_a=ay;old_b=by;
  }else if(page==ETCH_UI_MOD){
   uint32_t cy=etch_ui_wave_display_y(c[j],256,mid,extent,top+1,bottom-1);
   etch_scope_stroke(x,x==14u?cy:old_c,cy,MAGENTA);old_c=cy;
  }
  etch_scope_stroke(x,x==14u?y:old,y,WHITE);old=y;
 }
 if(draw){
  uint32_t grip=native_editor.cursor/4u;
  if(grip>31u)grip=31u;
  uint32_t cx=14u+(grip*212u+15u)/31u;
  lcd_fill(cx,top,1,bottom-top+1,LIME);
  uint32_t cy=etch_ui_wave_display_y(native_editor.points[native_editor.wave][native_editor.cursor],g,mid,extent,top+1,bottom-1);
  lcd_fill(cx>14u?cx-1u:cx,cy>top+1?cy-1:cy,3,3,LIME);
 }
 if(etch_save_toast_visible(etch_ms,native_saved_until)){
  /* High contrast confirmation drawn inside the same DMA graph frame.
   * Does not allocate, block the audio callback or touch sound state. */
  lcd_fill(69u,82u,102u,37u,BG);
  lcd_fill(69u,82u,102u,2u,LIME);
  lcd_fill(69u,117u,102u,2u,LIME);
  label_large("SAVED",80u,92u,WHITE);
 }
 etch_scope_commit();
 /* All visible knobs must invalidate the FX/ENV/ARP footer. V0.70 only
  * hashed REVERB, which left changed CHORUS/DELAY/DRIVE values stuck. */
 const uint16_t visible_values[]={
  native_controls.morph,native_controls.depth,native_editor.cursor,
  native_editor.wave,native_patch_slot,native_mod_ratio,native_cutoff,
  native_reverb,native_chorus,native_delay,native_drive,
  native_resonance,native_filter_type,native_env_ms[0],native_env_ms[1],
  native_env_ms[2],native_sustain,native_fx_settings.chorus_rate,
  native_fx_settings.chorus_depth,native_fx_settings.delay_time,
  native_fx_settings.delay_feedback,native_fx_settings.reverb_size,
  native_fx_settings.reverb_decay,native_fx_settings.routing,
  native_fx_settings.ring_mix,native_fx_settings.ring_rate,native_fx_settings.ring_shape,
  native_fx_settings.ring_bias,native_fx_settings.eq_low,native_fx_settings.eq_mid,
  native_fx_settings.eq_high,native_fx_settings.eq_hpf,
 native_fx_settings.comp_threshold,native_fx_settings.comp_ratio,native_fx_settings.comp_attack,native_fx_settings.comp_release,native_fx_settings.trem_rate,native_fx_settings.trem_depth,native_fx_settings.trem_shape,native_fx_settings.trem_mix,native_fx_settings.phaser_rate,native_fx_settings.phaser_depth,native_fx_settings.phaser_feedback,native_fx_settings.phaser_mix,native_fx_settings.flanger_rate,native_fx_settings.flanger_depth,native_fx_settings.flanger_feedback,native_fx_settings.flanger_mix,native_fx_settings.decim_bits,native_fx_settings.decim_rate,native_fx_settings.decim_drive,native_fx_settings.decim_mix,native_fx_settings.folder_fold,native_fx_settings.folder_symmetry,native_fx_settings.folder_tone,native_fx_settings.folder_mix,native_fx_settings.tape_wow,native_fx_settings.tape_flutter,native_fx_settings.tape_age,native_fx_settings.tape_mix,
   native_fx_settings.reverse_time,native_fx_settings.reverse_feed,native_fx_settings.reverse_tone,native_fx_settings.reverse_mix,
   native_fx_settings.vibrato_rate,native_fx_settings.vibrato_depth,native_fx_settings.vibrato_shape,native_fx_settings.vibrato_mix,
   native_fx_settings.rotary_rate,native_fx_settings.rotary_depth,native_fx_settings.rotary_drive,native_fx_settings.rotary_mix,
   native_fx_settings.wah_sens,native_fx_settings.wah_attack,native_fx_settings.wah_reso,native_fx_settings.wah_mix,
  native_arp_mode,native_arp_rate,native_arp_octaves,native_arp_gate_index,
  ETCH_V117_EXTRA_FOOTER_VALUES(native_fx_settings,native_project_cfg.evolution,native_pre_insert_slot),
  native_tempo,native_library.selection[2],native_mod_mode,native_free_millihz,native_controls.mod_target,native_page
 };
 uint32_t hash=etch_ui_values_fingerprint(visible_values,
          (uint8_t)(sizeof(visible_values)/sizeof(visible_values[0])));
 if(full || hash!=web_last_controls || native_tripped!=web_last_fault){
  /* 85-line footer is a second atomic RGB565 DMA transfer, including
   * effects/ADSR numerical values. No visibly erased individual labels. */
  etch_scope_begin(155u,85u);
  lcd_fill(0,155,240,85,BG);
  if(native_tripped)label("AUDIO STOP",14,161,0xf800);
  else if(native_boot_muted)label("SAFE MODE",14,161,AMBER);
  if(draw){
   label(native_editor.wave==0?"WAVE A":native_editor.wave==1?"WAVE B":"WAVE C",14,174,CYAN);
   label("SLOT",129,174,0x7bef);number(native_slot+1,174,174);
   label("POINT",14,192,0x7bef);number(native_editor.cursor/4u+1,69,192);
   label("VALUE",130,192,0x7bef);
   number(((int32_t)native_editor.points[native_editor.wave][native_editor.cursor]+32768)/512,183,192);
   label("K1 X K2 Y",14,213,LIME);
   label("K3 PART",14,228,0x7bef);number(native_harmonic,104,228);
   label("K4 HARM GAIN",138,228,MAGENTA);
  }else if(page==ETCH_UI_MORPH){
   label("WAVE A",14,177,CYAN);number(native_morph_a+1,85,177);
   label("WAVE B",130,177,AMBER);number(native_morph_b+1,198,177);
   label("K1 A  K2 B",14,196,LIME);
   label("K3 MORPH",14,213,WHITE);number(native_controls.morph*100u/32767u,114,213);
   label("ALG ETCH",14,229,MAGENTA);number(native_controls.depth*100u/32767u,106,229);
   label("K4 C RATE",143,213,MAGENTA);number(native_mod_ratio+1u,218,213);
  }else if(page==ETCH_UI_MOD){
   label("WAVE C",14,177,MAGENTA);number(native_library.selection[2]+1u,87,177);
   label("K1 WAVE K2 DEPTH",14,196,LIME);
   label("K3 RATE",14,213,WHITE);number(native_mod_mode==1?native_free_millihz/1000u:native_mod_ratio+1u,92,213);
   label("K4 TARGET",136,213,MAGENTA);number(native_controls.mod_target,219,213);
   label("SELECT MOD MODE",14,229,0x7bef);
  }else if(page==ETCH_UI_PLAY){
   label("A",14,172,CYAN);number(native_morph_a+1,29,172);
   label("B",91,172,AMBER);number(native_morph_b+1,106,172);
   label("C",167,172,MAGENTA);number(native_library.selection[2]+1,182,172);
   parameters();
  }else{parameters();}
  web_last_controls=hash;web_last_fault=native_tripped;
  etch_scope_commit();
 }
 web_last_page=page;web_last_draw=draw;web_last_signal=native_scope_has_signal;first_frame=0;
 lcd_sync();
}
static uint8_t web_primary_page(void){
 if(native_draw)return native_page==ETCH_UI_PLAY;
 return native_page==ETCH_UI_PLAY||native_page==ETCH_UI_ENV||native_page==ETCH_UI_MOD||
  native_page==ETCH_UI_FILTER||native_page==ETCH_UI_FX||native_page==ETCH_UI_ARP||
  native_page==ETCH_UI_MORPH||etch_ui_is_fx_editor(native_page)||
  native_page==ETCH_UI_PREFX_A||native_page==ETCH_UI_PREFX_B||native_page==ETCH_UI_C_FX||
  native_page==ETCH_UI_EVOLUTION;
}

/* 0.81: SEQ mirrors sim/screen.mjs rather than the old single-page
 * diagnostic grid. The 240x240 UI uses a 66-line header and a 174-line
 * offscreen panel, preserving the verified Felucca DMA window size. */
static uint8_t etch_seq_is_web_page(uint8_t page){
 return page==ETCH_UI_SEQ||page==ETCH_UI_NOTES||page==ETCH_UI_HARM;
}
static void etch_seq_web_panel(void){
 const uint16_t BG=0x0000,GRID=0x2124,WHITE=0xf7de,CYAN=0x67ff,
  AMBER=0xfde8,MAGENTA=0xf95b,LIME=0xdfe4,FAINT=0x7bef;
 const uint8_t page=native_page;
 const uint16_t display_step=etch_seq_actual_step(native_seq_run,native_audio.perf_last_seq,native_audio.perf_played,native_audio.perf_current_step,native_seq_length,native_seq_step);
 const struct etch_seq_step *st=&native_pattern.steps[display_step];
 static const char *const rates[4]={"1/4","1/8","1/16","1/32"};
 /* The native graph and note editor are composed before LCD transfer. */
 etch_scope_begin(0u,66u);
 lcd_fill(0,0,240,66u,BG);
 label_large("ETCH",12,7,LIME);
 label(page==ETCH_UI_SEQ?"SEQ GRID":page==ETCH_UI_NOTES?"SEQ STEP":"SEQ HARMONY",110,11,WHITE);
 label(ETCH_UI_VERSION,198,33,FAINT);
 lcd_fill(12,46,216,1,GRID);
 label(page==ETCH_UI_SEQ?"GRID":page==ETCH_UI_NOTES?"STEP":"HARMONY",14,52,CYAN);
 label(native_seq_recording?"REC":native_seq_run?"PLAY":"STOP",177,52,
       native_seq_recording?MAGENTA:native_seq_run?LIME:FAINT);
 etch_scope_commit();
 etch_panel_begin(67u);
 if(page==ETCH_UI_SEQ){
  uint8_t cols=etch_seq_parity_columns(native_seq_length);
  uint8_t rows=etch_seq_parity_rows(native_seq_length);
  uint8_t hold_count[ETCH_SEQ_STEPS]={0};
  etch_seq_parity_hold_counts(&native_pattern,native_seq_length,hold_count);
  /* Same web colour language: cyan trigger, amber continuing notes,
   * magenta parameter locks and bright selected/playhead border. */
  for(uint16_t i=0;i<native_seq_length;i++){
   uint32_t col=i%cols,row=i/cols;
   uint32_t left=14u+(col*212u)/cols;
   uint32_t right=14u+((col+1u)*212u)/cols;
   uint32_t top=70u+(row*121u)/rows;
   uint32_t bottom=70u+((row+1u)*121u)/rows;
   if(right<=left||bottom<=top)continue;
   uint32_t w=right-left,h=bottom-top;
   /* Single border via a dark inset; no per-cell DMA. */
   /* Draw all four boundaries, including the final column and bottom
    * row. The old left+top-only lines left the final steps open-ended. */
   lcd_fill(left,top,w,1,GRID);
   lcd_fill(left,bottom-1u,w,1,GRID);
   lcd_fill(left,top,1,h,GRID);
   lcd_fill(right-1u,top,1,h,GRID);
   if(hold_count[i])lcd_fill(left+2,top+2,w>4?w-4:1,1,AMBER);
   if(native_pattern.steps[i].count)lcd_fill(left+2,bottom-2,w>4?w-4:1,1,CYAN);
   if(native_pattern.steps[i].lock_mask)lcd_fill(right-3,top+2,2,2,MAGENTA);
   if(i==display_step){
    lcd_fill(left,top,w,1,native_seq_recording?MAGENTA:LIME);
    lcd_fill(left,bottom-1,w,1,native_seq_recording?MAGENTA:LIME);
    lcd_fill(left,top,1,h,native_seq_recording?MAGENTA:LIME);
    lcd_fill(right-1,top,1,h,native_seq_recording?MAGENTA:LIME);
   }
  }
  label("STEPS",14,200,FAINT);number(native_seq_length,68,200);
  label("RATE",127,200,FAINT);label(rates[native_seq_rate&3u],174,200,WHITE);
  label("TRIG",14,216,CYAN);label("HOLD",76,216,AMBER);label("LOCK",138,216,MAGENTA);
  label("S",14,231,LIME);number(display_step+1u,25,231);
  label("SW",63,231,FAINT);number(native_seq_swing,87,231);
  label("G",126,231,FAINT);number(native_seq_gate,139,231);
  label("BPM",172,231,FAINT);number(native_tempo,204,231);
 }else if(page==ETCH_UI_NOTES){
  struct etch_step_layers layers={0};
  etch_seq_layers(&native_pattern,native_seq_length,display_step,&layers);
  label("STEP",14,76,LIME);number(display_step+1u,61,76);
  label("/",98,76,FAINT);number(native_seq_length,114,76);
  label("TRIGGER",14,94,CYAN);number(st->count,95,94);
  label("HOLD",142,94,AMBER);number(layers.hold_pitches,192,94);
  for(uint8_t i=0;i<st->count && i<8;i++){
   uint32_t x=14u+(i%4u)*54u,y=112u+(i/4u)*25u;
   lcd_fill(x,y,50u,23u,i==native_note_sel?0x18a4:0x1082);
   label(etch_note_name(st->notes[i].note%12),x+3,y+3,i==native_note_sel?LIME:CYAN);
   number(st->notes[i].note/12u,x+26,y+3);
   if(st->notes[i].length>1u){label("L",x+4,y+14,AMBER);number(st->notes[i].length,x+14,y+14);}
  }
  if(!st->count)label("PLAY KEYS TO ADD NOTES",14,118,WHITE);
  label("LOCKS",14,170,FAINT);
  if(!st->lock_mask)label("NONE",76,170,FAINT);
  else {
   static const char *const lock_names[6]={"MORPH","ETCH","CUT","CHOR","DELAY","REV"};
   for(uint8_t j=0,k=0;j<6;j++)if(st->lock_mask&(1u<<j)){
    uint32_t x=75u+(k%3u)*52u,y=168u+(k/3u)*15u;
    label(lock_names[j],x,y,MAGENTA);k++;
   }
  }
  label("RATE",14,201,FAINT);label(rates[native_seq_rate&3u],57,201,WHITE);
  label("SW",116,201,FAINT);number(native_seq_swing,145,201);
  label("GATE",179,201,FAINT);number(native_seq_gate,211,201);
  label("SEL GRID",14,221,LIME);
  label("K1 VOICE K2 VEL",14,233,FAINT);
 }else{ /* HARMONY: root and scale are always actual notes, never 0..11 */
  struct etch_harmony_guide guide={0};
  etch_harmony_guide_build(&native_harmony,(uint8_t)(60u+native_harmony.root),&guide);
  label("KEY",14,77,FAINT);label(etch_note_name(native_harmony.root),54,77,LIME);
  label(etch_mode_name(native_harmony.mode),107,77,WHITE);
  label("KEYLOCK",14,99,FAINT);label(etch_lock_name(native_harmony.keylock),102,99,CYAN);
  label("CHORD",14,120,FAINT);number(native_harmony.chord,83,120);
  label("SCALE NOTES",14,141,FAINT);
  uint8_t shown=0;
  for(uint8_t n=0;n<12;n++){
   uint8_t pc=(uint8_t)((native_harmony.root+n)%12u);
   if(guide.scale_mask&(1u<<pc)){
    uint32_t x=14u+(shown%6u)*35u,y=156u+(shown/6u)*16u;
    label(etch_note_name(pc),x,y,WHITE);shown++;
   }
  }
  label("K1 KEY K2 SCALE",14,195,FAINT);
  label("K3 LOCK K4 CHORD",14,212,FAINT);
  label("SEL BAKE",14,229,LIME);
 }
 etch_panel_commit();first_frame=0u;
}
/* Dedicated named PRESETS screen: seven 18px rows fit into the 240x240
 * display; composed in the same offscreen LCD surfaces as every other page.
 * The preset names are read from the EXISTING 32 patch records. No writes. */
static void native_presets_render(void){
 const uint16_t WHITE=0xf7de, CYAN=0x67ff, FAINT=0x7bef, BG=0x0000, LIME=0xdfe4;
 etch_scope_begin(0u,66u);
 lcd_fill(0,0,240,66u,BG);
 label_large("ETCH",12,7,LIME);label("PRESETS",119,11,WHITE);
 label(ETCH_UI_VERSION,199,33,FAINT);
 lcd_fill(12,46,216,1,0x2945);
 label("PRESETS KNOB: SCROLL",12,52,CYAN);
 etch_scope_commit();
 etch_panel_begin(67u);
 lcd_fill(0,67,240,173u,BG);
 uint8_t visible=0;
 uint8_t slots[ETCH_PATCH_SLOTS];
 for(uint8_t s=0;s<ETCH_PATCH_SLOTS;s++)if(native_library.patches[s].valid)slots[visible++]=s;
 if(!visible){
  label("NO SAVED PRESETS",18,126,WHITE);
  label("SAVE A PRESET FIRST",18,147,FAINT);
 }else{
  uint8_t sel=0;
  for(uint8_t i=0;i<visible;i++)if(slots[i]==native_presets_cursor){sel=i;break;}
  for(uint8_t row=0;row<ETCH_PRESETS_ROWS;row++){
   int16_t n=(int16_t)sel+(int16_t)row-3;
   if(n<0||n>=visible)continue;
   uint8_t slot=slots[n],y=(uint8_t)(ETCH_PRESETS_ROW_Y+row*ETCH_PRESETS_ROW_H);
   uint8_t active=slot==native_patch_slot,selected=slot==native_presets_cursor;
   if(selected)lcd_fill(9,y-2,222,17,0x18a4);
   char name[13];
   etch_presets_name_copy(name,native_library.patches[slot].name);
   if(selected)label(">",12,y,LIME);
   number(slot+1u,29,y);label(name,65,y,selected?WHITE:FAINT);
   if(active)label("*",218,y,CYAN);
  }
 }
 lcd_fill(12,213u,216u,1u,0x2945);
 label("TURN: LOAD / HOME: EXIT",14,221,LIME);
 etch_panel_commit();
 first_frame=0u;
}
static void screen_render(void){
 /* Until the analog output has settled, leave the splash on the LCD.
  * Never accidentally hide Safe Mode, audio-fault or OTA diagnostics. */
 if(!native_screen_gate.ready && !native_safe && !native_tripped && !native_updating)return;
 if(native_page==ETCH_UI_PRESETS && !native_draw && !native_safe){native_presets_render();return;}
 if(!native_draw && etch_seq_is_web_page(native_page)){
  etch_seq_web_panel();return;
 }
 if(web_primary_page()){web_ui_primary();return;}
 /* SAVE is a complete, self-contained screen. The old renderer drew an
  * oscilloscope behind it and repeatedly erased the graph under the name
  * editor, causing tearing and visible flicker on the 240x240 LCD. */
 if(native_page==ETCH_UI_NAME||native_page==ETCH_UI_PROJECT_SAVE){
  const uint16_t BG=0x0000,CYAN=0x07ff,FAINT=0x7bef,WHITE=0xffff,AMBER=0xfd20;
  static uint8_t cache_page=255u,cache_slot=255u,cache_pos=255u,cache_warning=255u;
  static int cache_status=999;
  static char cache_name[13];
  uint8_t page=native_page;
  uint8_t slot=page==ETCH_UI_NAME?native_slot:(native_save_preset_mode?native_preset_save_slot:native_project_browser);
  uint8_t warning=(uint8_t)(native_save_missing_name?1u:
     (page==ETCH_UI_PROJECT_SAVE?native_save_overwrite:native_save_confirm)?2u:
     (native_save_attempted&&native_store_status!=ETCH_STORE_OK)?3u:0u);
  uint8_t redraw=first_frame||cache_page!=page;
  if(redraw){
   /* Header redraw is atomic too: never erase the whole LCD first. */
   etch_scope_begin(0u,66u);
   lcd_fill(0,0,240,66,BG);text_etch();
   label(ETCH_UI_VERSION,166,20,AMBER);label("DRAW MORPH MODULATE",12,51,FAINT);
   lcd_fill(12,63,216,1,0x2945);
   etch_scope_commit();
   cache_page=page;
   first_frame=0;
  }
  /* Static labels and all editable fields are drawn into one RAM surface.
   * The display sees only the finished frame, including backspace/overwrite. */
  etch_panel_begin(67u);
  label(page==ETCH_UI_NAME?"SAVE WAVEFORM":(native_save_preset_mode?"SAVE PRESET":"SAVE PROJECT"),12,73,WHITE);
  if(page==ETCH_UI_PROJECT_SAVE)label("SELECT: PROJECT / PRESET",12,92,CYAN);
  lcd_fill(12,88,216,1,0x2945);
  label("SLOT",12,103,FAINT);label("NAME",12,126,FAINT);
  lcd_fill(12,159,216,1,0x2945);
  label("K1 SLOT  K2 POSITION",12,203,FAINT);
  label("K3 LETTER  K4 BACKSPACE",12,220,FAINT);
  cache_slot=255u;cache_pos=255u;cache_warning=255u;cache_status=999;
  if(cache_slot!=slot){
   lcd_fill(72,102,130,10,BG);number((uint32_t)slot+1u,73,103);
   cache_slot=slot;
  }
  uint8_t name_changed=0;
  for(uint8_t i=0;i<12u;i++)if(cache_name[i]!=native_save_name[i])name_changed=1u;
  if(name_changed||cache_pos!=native_name_pos){
   lcd_fill(10u,132u,225u,21u,BG);
   for(uint8_t i=0;i<12;i++){
    char single[2]={native_save_name[i],0};uint32_t x=12u+18u*i;
    if(i==native_name_pos)lcd_fill(x-1u,144u,15u,2u,AMBER);
    label(single,x,134,WHITE);cache_name[i]=native_save_name[i];
   }
   cache_pos=native_name_pos;
  }
  if(cache_warning!=warning||cache_status!=native_store_status){
   lcd_fill(11,164,228,29,BG);
   if(native_save_missing_name)label("ENTER A NAME",12,167,0xf800);
   else if(page==ETCH_UI_PROJECT_SAVE?native_save_overwrite:native_save_confirm){
    label("OVERWRITE?",12,167,AMBER);
    label("SAVE YES / HOME NO",12,184,WHITE);
   }else if(native_save_attempted&&native_store_status!=ETCH_STORE_OK){
    label(native_store_status==ETCH_STORE_FOREIGN?"STORAGE LOCKED":
          native_store_status==ETCH_STORE_FULL?"STORAGE FULL":
          native_store_status==ETCH_STORE_INCOMPATIBLE?"STORAGE FORMAT":
          native_store_status==ETCH_STORE_IO?"STORAGE I/O ERROR":
          native_store_status==ETCH_STORE_VERIFY?"VERIFY FAILED":"SAVE FAILED",12,167,0xf800);
    label("NOT SAVED - GLO SYSTEM",12,184,AMBER);
   }else label("SAVE: CONFIRM",12,176,CYAN);
   cache_warning=warning;cache_status=native_store_status;
  }
  etch_panel_commit();return;
 }
 /* V0.77: Dedicated full-frame layouts. Do not draw an oscilloscope and
  * generic controls beneath the GUIDE / PROJECTS interface: both caused
  * extra LCD transfers and visually overlapping labels on 240x240 TFT. */
 if(!native_draw && (native_page==ETCH_UI_GUIDE||native_page==ETCH_UI_PROJECTS)){
  const uint16_t BG=0x0000,CYAN=0x07ff,FAINT=0x7bef,WHITE=0xffff,AMBER=0xfda0;
  if(first_frame){etch_scope_begin(0u,66u);
   lcd_fill(0,0,240,66,BG);text_etch();
   label(ETCH_UI_VERSION,166,20,0xffe0);label("DRAW MORPH MODULATE",12,51,FAINT);
   lcd_fill(12,63,216,1,0x2945);etch_scope_commit();first_frame=0;
  }
  /* Draw all changing labels and cells into RAM before one LCD blit. */
  etch_panel_begin(67u);
  lcd_fill(0,66,240,174,BG);
  label(native_page==ETCH_UI_GUIDE?"GUIDE":"PROJECTS",12,69,WHITE);
  lcd_fill(12,79,216,2,0x2945);
  if(native_page==ETCH_UI_GUIDE){
   const struct etch_harmony_settings *h=&native_harmony;
   label(native_guide_performance?"SCALE GUIDE":"CHORD BUILDER",12,87,CYAN);
   label("KEY",12,106,FAINT);label(etch_note_name(h->root),49,106,WHITE);
   label("SCALE",91,106,FAINT);label(etch_mode_name(h->mode),141,106,AMBER);
   struct etch_harmony_guide guide;
   if(!etch_harmony_guide_build(h,native_guide_note,&guide)){
    label("CHROMATIC C TO B",12,124,FAINT);
    for(uint8_t i=0;i<12;i++){
     /* A fixed C..B keyboard. Do NOT rotate the physical key positions by
      * the chosen root: C# must always be the second chromatic column. */
     uint8_t pc=etch_guide_pitch_at_column(i);
     uint8_t in_scale=(uint8_t)((guide.scale_mask>>pc)&1u);
     uint8_t in_chord=(uint8_t)((guide.chord_mask>>pc)&1u);
     uint16_t color=in_chord?AMBER:(in_scale?0x0499:0x2104);
     uint32_t x=etch_guide_column_x(i);
     lcd_fill(x,137u,ETCH_GUIDE_KEY_WIDTH,20u,color);
     /* Mark the black keys by a shorter top bar, without moving their labels. */
     if(etch_guide_is_accidental(pc))lcd_fill(x,137u,ETCH_GUIDE_KEY_WIDTH,3u,0x0000);
     label(etch_note_name(pc),x,145u,(in_chord||in_scale)?0x0000:WHITE);
     if(pc==h->root%12u)lcd_fill(x,160u,ETCH_GUIDE_KEY_WIDTH,2u,WHITE);
    }
    label("CHORD",12,170,FAINT);
    for(uint8_t i=0;i<guide.count&&i<4;i++)
     label(etch_note_name(guide.notes[i]),76u+i*39u,170u,WHITE);
   }
   label(native_guide_performance?"GLO: FIFTHS":"SEQ: REC BAKE",12,184,CYAN);
   lcd_fill(0,195,240,1,0x2945);
   /* All four labels/values occupy separate 60px cells, none exceed 7 glyphs. */
   label("K1 KEY",4,201,FAINT);label("K2 SCL",64,201,FAINT);
   label("K3 LOCK",124,201,FAINT);label("K4 NOTE",184,201,FAINT);
   label(etch_note_name(h->root),4,220,WHITE);
   label(etch_mode_short(h->mode),64,220,WHITE);
   label(etch_lock_name(h->keylock),124,220,WHITE);
   label(etch_note_name(native_guide_note%12u),184,220,WHITE);
  }else{
   label("PROJECT BROWSER",12,87,CYAN);
#if ETCH_P05_MULTI
   for(uint8_t i=0;i<4;i++){
    uint32_t y=ETCH_PROJECT_UI_FIRST_Y+(uint32_t)i*ETCH_PROJECT_UI_ROW_STEP;
    uint8_t selected=i==native_project_browser;
    if(selected)lcd_fill(7,y-2,226,16,0x18a3);
    char slot[4]={(char)('1'+i),'.',' ',0};
    label(slot,12,y,selected?WHITE:FAINT);
    label((native_project_present&(1u<<i))?native_project_names[i]:"EMPTY",42,y,selected?WHITE:FAINT);
   }
   if(native_project_browser==4u)lcd_fill(7,ETCH_PROJECT_UI_NEW_Y-2,226,16,0x18a3);
   label((native_project_present&15u)==15u?"PROJECT SLOTS FULL":"NEW PROJECT",
         12,ETCH_PROJECT_UI_NEW_Y,native_project_browser==4u?WHITE:CYAN);
   lcd_fill(0,195,240,1,0x2945);
   if(native_store_status==ETCH_STORE_INCOMPATIBLE && native_project_browser==etch_store_project_selected())
    label("OPEN OTHER TO DELETE",8,ETCH_PROJECT_UI_HINT_Y,AMBER);
   else if(native_project_armed==3u){label("SEL+SAVE: DELETE?",8,ETCH_PROJECT_UI_HINT_Y,AMBER);}
   else if(native_project_browser==4u){label("CREATE NEW PROJECT?",8,ETCH_PROJECT_UI_HINT_Y,WHITE);}
   else if(native_project_present&(1u<<native_project_browser)){
    label("OPEN SELECTED PROJECT?",8,ETCH_PROJECT_UI_HINT_Y,WHITE);
   }else label("EMPTY PROJECT SLOT",8,ETCH_PROJECT_UI_HINT_Y,FAINT);
   label(native_project_armed==3u?"SEL+SAVE CONFIRM":"SEL YES  HOME BACK",
         8,ETCH_PROJECT_UI_CONFIRM_Y,CYAN);
#else
   label("PROJECTS DISABLED",12,130,AMBER);
#endif
  }
  etch_panel_commit();return;
 }

 if(first_frame){etch_scope_begin(0u,66u);
  lcd_fill(0,0,240,66,0x0000);text_etch();label(ETCH_UI_VERSION,166,20,0xffe0);
  label("DRAW MORPH MODULATE",12,51,0x7bef);
  lcd_fill(12,63,216,1,0x2945);
  if(native_boot_muted)label("MUTE",190,34,0xffe0);
  etch_scope_commit();first_frame=0;}
 etch_panel_begin(67u);
 lcd_fill(12,68,216,7,0);
 if(native_safe){label("SAFE",12,68,0xFFE0);
  if(native_recovery_reason==ETCH_RECOVERY_STORAGE_FAULT)label("STORE FAIL",90,68,0xF800);
  else if(native_recovery_reason==ETCH_RECOVERY_BOOT_LOOP)label("BOOT FAIL",90,68,0xF800);
  else if(native_recovery_reason==ETCH_RECOVERY_OTA_CLEANUP)label("OTA FAIL",90,68,0xF800);
  else label("REC MODE",90,68,0xFFE0);}else if(native_page==8){label("NAME",12,68,0xFFFF);}else if(native_page==9){label(native_tool==12u?"GENETICS":"TOOLS",12,68,0xFFFF);}else if(native_draw){label(native_page==14?"WFX 1":native_page==15?"WFX 2":"DRAW",12,68,0xFFFF);char wave[2]={(char)('A'+native_editor.wave),0};label(wave,56,68,0xFFE0);}else label(etch_ui_title(native_page),12,68,0xFFFF);
 if(native_draw){
  char slot[4]={'S',(char)('0'+native_slot/10),(char)('0'+native_slot%10),0};label(slot,80,68,0xFFFF);
  const char *state=!ETCH_NATIVE_STORAGE?"RAM":native_save_confirm?"SURE":native_store_status==ETCH_STORE_OK?"OK":native_store_status==ETCH_STORE_EMPTY?"EMPTY":native_store_status==ETCH_STORE_FOREIGN?"LOCK":native_store_status==ETCH_STORE_FULL?"FULL":"FAIL";
  label(native_init_held?"CLEAR":state,130,68,native_store_status<0?0xF800:0xFFFF);
 }
 int16_t scope[128];fm1_irq_off();for(uint32_t i=0;i<128;i++)scope[i]=native_scope[native_scope_ready][i];fm1_irq_enable_all();
 /* PLAY uses the previously unused gap above and below the scope. Other
  * pages retain their layout because they have controls within this area. */
 const uint8_t scope_top=native_page==0&&!native_draw?79:85;
 const uint8_t scope_bottom=native_page==0&&!native_draw?193:185;
 const uint8_t scope_mid=(uint8_t)((scope_top+scope_bottom)/2);
 const uint8_t scope_extent=(uint8_t)((scope_bottom-scope_top-3)/2);
 etch_scope_begin(scope_top,(uint16_t)(scope_bottom-scope_top+1));
 lcd_fill(0,scope_top,240,(uint32_t)(scope_bottom-scope_top+1),0x0000);
 for(uint32_t x=0;x<240;x+=24)lcd_fill(x,scope_top,1,scope_bottom-scope_top,0x2124);
 for(uint32_t k=0;k<=4;k++)lcd_fill(0,scope_top+(scope_bottom-scope_top)*k/4,240,1,0x2124);
 int16_t shaped[128];
 if(native_draw&&(native_page==14||native_page==15)){
  etch_wave_fx_apply(native_editor.points[native_editor.wave],&native_editor.fx[native_editor.wave],shaped);
  uint32_t prior=scope_mid;
  for(uint32_t x=0;x<240;x++){
   uint32_t y=etch_ui_wave_display_y(shaped[x*128u/240u],256u,scope_mid,scope_extent,scope_top+1u,scope_bottom-1u);
   etch_scope_stroke(x,prior,y,0xFDA0);prior=y;
  }
 }
 if(!native_draw&&(native_page==23||native_page==24)){
  const int16_t *a=native_editor.points[0],*b=native_editor.points[1];
  if(native_page==23){
   if(etch_ui_wave_valid(native_library.valid,native_morph_a))a=native_library.slot[native_morph_a];
   if(etch_ui_wave_valid(native_library.valid,native_morph_b))b=native_library.slot[native_morph_b];
  }else if(etch_ui_wave_valid(native_library.valid,native_browser_slot)){
   a=native_library.slot[native_browser_slot];b=a;
  }
  etch_ui_wave_morph(a,b,native_page==23?native_controls.morph:0,shaped);
  const uint16_t ag=etch_ui_wave_display_gain(a,128),bg=etch_ui_wave_display_gain(b,128);
  /* One shared scale preserves the relative amplitudes of A, B and result. */
  const uint16_t gain=ag<bg?ag:bg;
  uint32_t prev_a=scope_mid,prev_b=scope_mid,prev_m=scope_mid;
  for(uint32_t x=0;x<240;x++){
   uint32_t j=x*128u/240u;
   uint32_t ay=etch_ui_wave_display_y(a[j],gain,scope_mid,scope_extent,scope_top+1,scope_bottom-1);
   uint32_t by=etch_ui_wave_display_y(b[j],gain,scope_mid,scope_extent,scope_top+1,scope_bottom-1);
   uint32_t my=etch_ui_wave_display_y(shaped[j],gain,scope_mid,scope_extent,scope_top+1,scope_bottom-1);
   if(native_page==23){etch_scope_stroke(x,prev_a,ay,0x07ff);etch_scope_stroke(x,prev_b,by,0xfda0);}
   etch_scope_stroke(x,prev_m,my,0xffff);
   prev_a=ay;prev_b=by;prev_m=my;
  }
  if(native_page==24){
   char name[13];for(uint8_t i=0;i<12;i++)name[i]=native_library.wave_names[native_browser_slot][i];name[12]=0;
   if(etch_ui_wave_valid(native_library.valid,native_browser_slot))label(name,12,91,0xfda0);
   else label("EMPTY",12,91,0xf800);
  }
  if(native_page==23){label("EDIT A B C",12,158,0x7bef);
   label(native_morph_focus==0?"FOCUS A":native_morph_focus==1?"FOCUS B":"FOCUS C",148,158,0xfda0);
  }
  label(native_wave_load_error?"EMPTY SLOT":native_wave_load_armed?"REC AGAIN":"REC LOAD",12,174,native_wave_load_error?0xf800:0x07ff);
 }else{
  const int16_t *visual=native_draw?native_editor.points[native_editor.wave]:scope;
  int32_t offset=native_draw?0:etch_ui_scope_midpoint(visual,128);
  const uint16_t gain=native_draw?etch_ui_wave_display_gain(visual,128)
                       :etch_ui_scope_centered_gain(visual,128,offset);
  uint32_t previous=etch_ui_wave_display_y(etch_ui_scope_centered(visual[0],offset),gain,scope_mid,scope_extent,scope_top+1,scope_bottom-1);
  for(uint32_t x=0;x<240;x++){
   uint32_t y=etch_ui_wave_display_y(etch_ui_scope_centered(visual[x*128u/240u],offset),gain,scope_mid,scope_extent,scope_top+1,scope_bottom-1);
   etch_scope_stroke(x,previous,y,0xFFFF);previous=y;
  }
 }
 if(!native_draw&&native_page==2){
  /* A stable preview of Wave C, separate from the audible scope above. */
  const int16_t *c=native_editor.points[2];
  uint32_t prior=scope_mid;
  for(uint32_t x=0;x<240;x++){
   uint32_t y=etch_ui_wave_display_y(c[x*128u/240u],256u,scope_mid,scope_extent,scope_top+1,scope_bottom-1);
   etch_scope_stroke(x,prior,y,0xf81f);prior=y;
  }
  label("C LIB",12,88,0xf81f);
  number(etch_ui_wave_ordinal(native_library.valid,native_library.selection[2]),64,88);
  label("OF",93,88,0x7bef);
  number(etch_ui_wave_count(native_library.valid),119,88);
  label("SLOT",153,88,0x7bef);
  number(native_library.selection[2]+1u,195,88);
 }
 etch_scope_commit();
 if(native_page==ETCH_UI_NAME||native_page==ETCH_UI_PROJECT_SAVE){
  lcd_fill(0,86,240,103,0);
  label(native_page==ETCH_UI_NAME?"GLOBAL WAVE":"PROJECT",10,92,0x07ff);
  label("NAME",10,111,0x7bef);label(native_save_name,60,111,0xffff);
  /* One visible cursor for all twelve character positions. */
  lcd_fill(60u+(uint32_t)native_name_pos*8u,124u,7u,2u,0xfde8);
  label("SLOT",10,129,0x7bef);
  number((native_page==ETCH_UI_NAME?native_slot:native_project_browser)+1u,60,129);
  label(native_page==ETCH_UI_NAME?"64 WAVE SLOTS":"4 PROJECT SLOTS",10,148,0x7bef);
  if(native_save_missing_name)label("ENTER A NAME",10,165,0xf800);
  else if(native_page==ETCH_UI_PROJECT_SAVE?native_save_overwrite:native_save_confirm){
   label("OVERWRITE?",10,165,0xffa0);
   label("SAVE YES HOME NO",105,165,0xffff);
  }else label("K1 SLOT K2 POS",10,165,0x07ff);
 }
 if(!native_draw&&native_page==27){
  lcd_fill(0,85,240,101,0);
#if ETCH_P05_MULTI
  label("PROJECT BROWSER",12,88,0x07ff);
  for(uint32_t i=0;i<4;i++){
   uint16_t tone=i==native_project_browser?0xffe0:0x7bef;
   char index[3]={(char)('1'+i),':',0};label(index,12,109+i*17u,tone);
   label((native_project_present&(1u<<i))?native_project_names[i]:"EMPTY",42,109+i*17u,tone);
  }
  label(native_project_browser==4u?((native_project_present&15u)==15u?"PROJECT SLOTS FULL":"> NEW PROJECT"):"REC LOAD",12,178,native_project_browser==4u?0xffe0:0x07ff);
  if(native_project_armed)label(native_project_armed==1?"REC TO LOAD":"CANCEL",105,90,0xfda0);
#else
  label("PROJECTS DISABLED",12,112,0xfda0);
#endif
 }
 if(!native_draw&&native_page==ETCH_UI_EVOLUTION){
  lcd_fill(0,86,240,97,0);
  label("LIVE WAVE EVOLUTION",12,88,0x07ff);
  static const char *const modes[3]={"FOLD","SHARP","STAIR"};
  static const char *const syncs[3]={"FREE","TEMPO","ONE SHOT"};
  label("SHAPE",12,112,0x7bef);label(modes[native_project_cfg.evolution.mode],88,112,0xf95b);
  label("SYNC",12,133,0x7bef);label(syncs[native_project_cfg.evolution.sync],88,133,0xdfe4);
  label("LFO: MOD > C FX > EVOLVE",12,159,0xffff);
  label(native_project_cfg.evolution.depth?"REC: EVOLVE OFF":"REC: EVOLVE ON",12,176,0x07e0);
 }
 if(!native_draw&&native_page==ETCH_UI_MOTION){
  lcd_fill(0,85,240,99,0);
  label("FOUR-LANE MOTION LOOP",12,88,0x07ff);
  label(native_motion_recording?"RECORDING":native_motion_playing?"PLAYING":"STOPPED",12,112,
        native_motion_recording?0xf81f:native_motion_playing?0x07e0:0x7bef);
  label("STEPS",12,133,0x7bef);number(native_project_cfg.motion.length,65,133);
  label("LANES",130,133,0x7bef);number(native_project_cfg.motion.mask,190,133);
  label("REC CAPTURE",12,157,0xffff);label("PLAY LOOP",125,157,0xffff);
 }
 if(!native_draw&&native_page==25){
  lcd_fill(0,85,240,101,0);
  if(native_system_tab==0){
   label("SYSTEM USB AUDIO",10,91,0x07ff);
   label(usb.config?(usb.suspended?"USB SUSPENDED":"USB CONNECTED"):"USB DISCONNECTED",10,110,usb.config?0x07e0:0xfda0);
#if ETCH_NATIVE_UAC
   label(uac.alt?(uac.flowing?"UAC STREAM":"UAC WAIT HOST"):"UAC IDLE",10,128,0xffff);
   label("UND",10,146,0x7bef);number(uac.underruns,50,146);
   label("OVER",125,146,0x7bef);number(uac.overruns,175,146);
   label("MISS",10,165,0x7bef);number(uac.missed,60,165);
   label("STALL",125,165,0x7bef);number(uac.stalls,182,165);
#else
   label("UAC DISABLED",10,142,0xfda0);
   label("REC MOTION PAGE",10,169,0x07ff);
#endif
  }else if(native_system_tab==1){
   label("STORAGE STATUS",10,91,0x07ff);
   if(native_store_status>=0 && etch_store_active_region()==3u)
    label("ETCH OWNED / A+B",10,105,0x07e0);
   else if(native_store_status>=0 && etch_store_active_region()==2u)
    label("LEGACY ETCH",10,105,0xfda0);
   label(native_store_status==ETCH_STORE_OK?"CRC OK":native_store_status==ETCH_STORE_EMPTY?"EMPTY":native_store_status==ETCH_STORE_FULL?"FULL":native_store_status==ETCH_STORE_FOREIGN?"FOREIGN":"CHECK",10,119,0xffff);
   struct etch_capacity_report c=etch_capacity_report(native_save_required,ETCH_PROJECT_MAX_BYTES);
   label("LAST",10,137,0x7bef);
   if(native_capacity_checked)number(c.required,72,137);
   else label("CHECK",72,137,0xfda0);
   label("MAX",10,157,0x7bef);number(c.available,72,157);
   if(native_capacity_checked){
    label(c.fits?"FITS":"OVER",10,177,c.fits?0x07e0:0xf800);
    if(!c.fits)number(c.overflow,72,177);
   }else label("SAVE TO TEST",10,177,0x7bef);
   if(native_store_status==ETCH_STORE_FOREIGN){
    lcd_fill(0,130,240,82,0);
    label(native_flash_probe.io_error?"FLASH READ ERROR":
          "ETCH BANK OCCUPIED",9,137,0xfda0);
    label("FORMAT ETCH STORAGE",9,153,0xffff);
    label("ERASES OLD USER DATA",9,170,0xf800);
    label("FX + SAVE: HOLD 5S",9,187,0xffff);
   }
  }else if(native_system_tab==2){
   label("RECOVERY STATUS",10,91,0x07ff);
   label("EXPERIMENTAL",10,119,0xfda0);
   label("NO VERIFIED RESTORE",10,142,0xf800);
   label("NO FLASH FROM MENU",10,169,0xffff);
  }else if(native_system_tab==3){
   label("PANEL DIAGNOSTIC",10,91,0x07ff);
   if(native_panel_capture){
    label("TEST ACTIVE",10,108,0x07e0);
    label("KEY LEFT",10,126,0x7bef);number(etch_diag_panel_missing_keys(&native_panel_diag),110,126);
    label("BTN LEFT",10,143,0x7bef);number(etch_diag_panel_missing_buttons(&native_panel_diag),110,143);
    label("ENC DIR LEFT",10,160,0x7bef);number(etch_diag_panel_missing_encoder_directions(&native_panel_diag),130,160);
    if(etch_diag_panel_complete(&native_panel_diag))label("ALL INPUTS SEEN",10,176,0x07e0);
    else{label("LED",10,176,0x7bef);number(1u+(etch_ms/160u)%27u,58,176);}
   }else{
    label("REC START TEST",10,115,0x07ff);
    label("27 KEYS 14 BTN 7 ENC",10,136,0x7bef);
    label("TURN EACH BOTH WAYS",10,155,0x7bef);
    label("HOME SAVE TO EXIT",10,174,0xfda0);
   }
  }else if(native_system_tab==4){
   label("AUDIO DIAGNOSTIC",10,91,0x07ff);
   label("DSP MAX",10,109,0x7bef);
   number(etch_diag_cpu_permille(native_max_ticks,ETCH_NATIVE_FRAMES*544u)/10u,110,109);
   label("WALL MAX",10,127,0x7bef);
   number(etch_diag_cpu_permille(native_max_wall_ticks,ETCH_NATIVE_FRAMES*544u)/10u,110,127);
   label("NEST MAX",10,145,0x7bef);
   number(etch_diag_cpu_permille(native_max_nested_ticks,ETCH_NATIVE_FRAMES*544u)/10u,110,145);
   label("CLIPS",10,163,0x7bef);number(native_audio_clip_frames,100,163);
   label("LATE",10,181,0x7bef);number(native_late,65,181);
   label("GUARD",123,181,0x7bef);number(native_dma_blocked,184,181);
   if(native_tripped)label("HOME PLAY RESTART",10,92,0xF800);
  }else{
   label("AUDIO LIVE",10,91,0x07ff);
   label("DMA",10,110,0x7bef);number(native_halves,94,110);
   label("OUTPUT",10,128,0x7bef);number(native_audio_peak,94,128);
   label("MASTER",10,146,0x7bef);number((uint32_t)native_controls.gain*100u/32767u,94,146);
   label("KEYS",10,164,0x7bef);number(fm1_in.notes,94,164);
   label(native_tripped?"STOPPED":"RUNNING",10,181,native_tripped?0xf800:0x07e0);
  }
 }
 if(!native_draw&&(native_page==6||native_page==10)){
  struct etch_step_layers layers;
  etch_seq_layers(&native_pattern,native_seq_length,
    etch_seq_actual_step(native_seq_run,native_audio.perf_last_seq,native_audio.perf_played,native_audio.perf_current_step,native_seq_length,native_seq_step),&layers);
  lcd_fill(0,147,240,38,0);
  label("TRIG",12,151,0x07ff);number(layers.trigger_count,52,151);
  label("HOLD",92,151,0xfda0);number(layers.hold_pitches,140,151);
  label("SAME",12,172,0xffff);number(layers.overlap_pitches,52,172);
 }
 if(!native_draw&&native_page==22){
  lcd_fill(0,86,240,101,0);
  const struct etch_arp_program *ap=&native_project_cfg.arp_programs[native_patch_slot];
  label("CUSTOM ORDER  /  REC REST",8,91,0x07ff);
  for(uint8_t a=0;a<16;a++){
   uint32_t x=4+(a%8)*29,y=111+(a/8)*34;
   lcd_fill(x,y,26,28,a==native_arp_edit_step?0x52aa:0x2104);
   if(ap->length&&a<ap->length){
    if(ap->steps[a]==255)label("R",x+8,y+8,0xfda0);
    else number(ap->steps[a]+1,x+4,y+8);
   }
  }
 }else if(!native_draw&&native_page==18){
  lcd_fill(0,86,240,100,0);
  uint16_t playing_step=etch_seq_actual_step(native_seq_run,native_audio.perf_last_seq,native_audio.perf_played,native_audio.perf_current_step,native_seq_length,native_seq_step);
  uint16_t first=(uint16_t)((playing_step/16)*16);
  for(uint8_t i=0;i<16;i++){
   uint16_t st=(uint16_t)(first+i);if(st>=native_seq_length)break;
   const struct etch_seq_step *q=&native_pattern.steps[st];
   uint32_t x=3+(i%8)*29,y=94+(i/8)*42;
   uint16_t bg=st==playing_step?0x52aa:0x1082;
   lcd_fill(x,y,26,35,bg);
   if(q->count)lcd_fill(x,y,26,3,0x07ff);
   if(q->hold_count)lcd_fill(x,y+31,26,4,0xfda0);
   number(st+1,x+2,y+8);
   if(q->lock_mask)lcd_fill(x+20,y+22,4,4,0xffff);
  }
 }else if(!native_draw&&native_page==20){
  lcd_fill(0,86,240,100,0);label("HOLD LANE",12,96,0xfda0);
  number(native_seq_step+1,12,118);label("PITCH",84,118,0xffff);number(native_hold_pitch,143,118);
  label("SRC THEN REC",12,144,0xfda0);
  if(native_edit_feedback)label(native_edit_feedback==1?"ON":native_edit_feedback==2?"OFF":"FULL",12,171,native_edit_feedback==3?0xf800:0x07e0);
 }else if(!native_draw&&native_page==26){
  /* Compact 240x240 fifths ring: highlighted root and scale members. */
  static const uint8_t px[12]={111,157,188,196,188,157,111,65,35,26,35,65};
  static const uint8_t py[12]={92,97,110,130,149,164,169,164,149,130,110,97};

  struct etch_harmony_guide preview;
  lcd_fill(0,86,240,101,0);
  if(!etch_harmony_guide_build(&native_harmony,(uint8_t)(60+native_harmony.root),&preview)){
   for(uint8_t i=0;i<12;i++){
    uint8_t pc=etch_ui_fifths_pitch(i);
    uint16_t color=pc==native_harmony.root?0xfda0:((preview.scale_mask&(1u<<pc))?0x07ff:0x4208);
    lcd_fill(px[i],py[i],10,10,color);
    label(etch_note_name(pc),px[i]+11,py[i],pc==native_harmony.root?0xffff:0x7bef);
   }
  }
  label("FIFTHS",83,118,0xffff);
  label(etch_note_name(native_harmony.root),105,133,0xffff);
  label(etch_mode_name(native_harmony.mode),66,149,0xfda0);
  label("K1 ROTATE FIFTHS",55,165,0x7bef);
 }else if(!native_draw&&native_page==19){
  lcd_fill(0,86,240,100,0);label(native_guide_performance?"SCALE GUIDE":"CHORD BUILDER",10,91,0x07ff);
  label("KEY",10,103,0x7bef);label(etch_note_name(native_harmony.root),47,103,0xffff);
  label(etch_mode_name(native_harmony.mode),84,103,0xfda0);
  struct etch_harmony_guide preview;
  if(!etch_harmony_guide_build(&native_harmony,native_guide_note,&preview)){
   for(uint8_t i=0;i<12;i++){
    uint8_t pc=etch_guide_pitch_at_column(i);
    uint8_t in_scale=(uint8_t)((preview.scale_mask>>pc)&1u);
    uint8_t in_chord=(uint8_t)((preview.chord_mask>>pc)&1u);
    uint16_t color=in_chord?0xfda0:(in_scale?0x0499:0x2104);
    uint32_t x=etch_guide_column_x(i);
    lcd_fill(x,109,ETCH_GUIDE_KEY_WIDTH,25,color);
    if(etch_guide_is_accidental(pc))lcd_fill(x,109,ETCH_GUIDE_KEY_WIDTH,3,0);
    label(etch_note_name(pc),x,118,(in_scale||in_chord)?0:0xffff);
    if(pc==native_harmony.root%12u)lcd_fill(x,136,ETCH_GUIDE_KEY_WIDTH,2,0xffff);
   }
   label("CHORD",12,148,0xffff);
   for(uint8_t i=0;i<preview.count;i++)label(etch_note_name(preview.notes[i]),70+i*39,148,0xffff);
  }
  label(native_guide_performance?"GLO FIFTHS":"SEQ: REC BAKE",10,171,0x07ff);
  if(native_guide_performance){
   label("LOCK",115,171,0x7bef);
   label(etch_lock_name(native_harmony.keylock),158,171,
         native_harmony.keylock==2?0x07ff:0xfda0);
  }
 }else if(!native_draw&&native_page==21){
  lcd_fill(0,86,240,100,0);
  label(native_bake_scope?"ALL STEPS":"ONE STEP",10,94,0x07ff);
  label("CHANGED",10,124,0xffff);
  int changed=etch_seq_bake_harmony(&native_pattern,native_seq_length,native_seq_step,
     native_bake_scope,&native_harmony,0);
  if(changed>=0)number((uint32_t)changed,125,124);
  else label("INVALID",125,124,0xf800);
  label(native_bake_confirm?"REC CONFIRM":"REC PREVIEW",10,158,native_bake_confirm?0xfda0:0x07ff);
 }else if(!native_draw&&native_page==10){
  lcd_fill(0,86,240,100,0);const struct etch_seq_step *q=&native_pattern.steps[native_seq_step];
  for(uint8_t i=0;i<q->count;i++){
   uint8_t col=i%4,row=i/4;uint32_t x=5+col*58,y=102+row*36;
   lcd_fill(x,y,53,30,i==native_note_sel?0x52aa:0x2104);
   number(q->notes[i].note,x+4,y+5);number(q->notes[i].length,x+31,y+17);
  }
  if(!q->count)label("PLAY KEYS",12,118,0xffff);
  else if(!native_edit_feedback){label("VEL",12,175,0xffff);number(q->notes[etch_seq_note_cursor(q->count,native_note_sel)].velocity,48,175);}
  if(native_edit_feedback==2)label("DUP NOTE",12,177,0xf800);
  else if(native_edit_feedback==3)label("FULL",12,177,0xf800);
 }
 if(native_draw&&native_page!=8&&native_page!=9&&native_page!=14&&native_page!=15){
  /* 32 grips (0..31) map to the exact left/right pixel edges (0..239).
   * The last grip is sample 124, but its screen position is pixel 239. */
  uint32_t cx=etch_ui_draw_cursor_x(native_editor.cursor);
  lcd_fill(cx,scope_top,cx==239u?1u:2u,scope_bottom-scope_top,0xFFE0);
  lcd_fill(0,196,240,40,0x0000);
  label("K1 X",6,201,0x07ff);number(native_editor.cursor/4u+1u,56,201);
  label("K2 Y",116,201,0xf81f);number((uint32_t)((int32_t)native_editor.points[native_editor.wave][native_editor.cursor]+32768)/512,170,201);
  label("K3 PART",6,219,0x7bef);number(native_harmonic,83,219);
  label("K4 SHAPE",116,219,0xf81f);
 }else if(native_page==ETCH_UI_NAME||native_page==ETCH_UI_PROJECT_SAVE){
  lcd_fill(0,196,240,40,0);label("K3 LETTER",8,199,0x7bef);label("K4 LEFT BACKSPACE",101,199,0xfde8);
  label("SAVE AGAIN = STORE",8,218,0x07ff);
 }else if(native_page==ETCH_UI_SYSTEM){
  /* Diagnostics need their own footer; the generic four-knob parameter
   * footer would otherwise erase VOICE/SHED telemetry after drawing it. */
  lcd_fill(0,196,240,40,0x0000);
  if(native_system_tab==4){
   label("VOICE",10,204,0x7bef);number(native_audio.voice_limit,73,204);
   label("SHED",123,204,0x7bef);number(native_voice_shed_count,184,204);
  }else label("SELECT NEXT TAB",12,208,0x7bef);
 }else{
 parameters();
 }
 if(etch_save_toast_visible(etch_ms,native_saved_until)){
  lcd_fill(69u,82u,102u,37u,0);
  lcd_fill(69u,82u,102u,2u,0x07e0);
  lcd_fill(69u,117u,102u,2u,0x07e0);
  label_large("SAVED",80u,92u,0xffff);
 }
 if(native_seq_clear_armed&&(native_page==6||native_page==18))label("REC CLEAR",112,68,0xFFE0);
 else if(native_fx_bake_confirm)label("BAKE AGAIN",120,68,0xFFE0);
 else if(native_fx_warning)label("WFX RAM",120,68,0xF800);
 else if(native_store_status==ETCH_STORE_FULL)label("FULL",120,68,0xF800);
 else if(native_seq_warning)label("SEQ RAM",120,68,0xF800);
 else if(native_page==7&&native_patch_wave_warning)label("SAVE WAVES",120,68,0xF800);
 else if(native_page==ETCH_UI_ARP && native_arp_mode==7u)label("RIFF K4 GEN",120,68,0x07e0);
 else if(native_page==7&&native_patch_confirm)label("SAVE AGAIN",120,68,0xFFE0);
 if(native_page==ETCH_UI_PLAY&&!native_draw)label(native_mono_mode?"MONO":"POLY",192,68,0x07e0);
 lcd_fill(12,76,216,4,native_tripped?0xF800:native_boot_muted?0xFFE0:0x07E0);
 if(native_tripped)label("AUDIO STOP",135,68,0xF800);
 else if(native_screen_gate.timed_out)label("AUDIO WAIT",135,68,0xFFE0);
 etch_panel_commit();
}
/* Render the first usable PLAY screen with the analog amp already enabled,
 * but with DMA still emitting digital zero. Start the sample-wise PCM fade
 * only AFTER that LCD/SPI transfer finishes. Never fade from the audio ISR
 * while the first large LCD frame is competing for memory bandwidth. */
static void screen(void){
 uint8_t can_paint=(uint8_t)(native_screen_gate.ready && !native_screen_gate.timed_out &&
                            !native_safe && !native_tripped && !native_updating);
 /* Count the display duration from first committed UI draw, not from the
  * end of flash IO. This survives slow LCD or a delayed UI service tick. */
 if(native_save_notice_pending && native_screen_gate.ready){
  native_saved_until=etch_ms+2300u;
  native_save_notice_pending=0u;
 }
 screen_render();
 if(!native_boot_audio_released && can_paint && native_prepared_blocks>=24u && etch_output_may_unmute((uint8_t)(!native_suspended && !native_tripped),
                       native_output_analog_ready,native_screen_gate.ready,
                       native_screen_gate.timed_out,1u)){
  /* native_output_enabled is volatile and consumed by ALNK0. Set fade first. */
  native_output_fade=0u;
  native_output_enabled=1u;
  native_boot_audio_released=1u;
 }
}
void fm1_cstart(void){
 uint32_t *s,*d;
 /* Assert the board's PC6 analog-output gate at the very first C
  * instruction, before framebuffer/BSS setup, timers, display or USB.
  * The mask-ROM/power-on interval precedes our control. */
 fm1_audio_output_enable(0u);
 fm1_time_init();fm1_reset_reason();fm1_wdt_arm(0x0D);
 uint8_t safe=etch_boot_begin(&etch_boot);
 fm1_irq_init();
 for(d=_bss_start;d<_bss_end;d++)*d=0;
 for(d=_pool_start;d<_pool_end;d++)*d=0;
 for(s=_data_load,d=_data_start;d<_data_end;s++,d++)*d=*s;
 for(s=_rt_load,d=_rt_start;d<_rt_end;s++,d++)*d=*s;
 native_safe=safe;
 native_recovery_reason=safe?ETCH_RECOVERY_BOOT_LOOP:ETCH_RECOVERY_NORMAL;
 fm1_mailbox_clear();fm1_guard_enable(FM1_GUARD_STACK|FM1_GUARD_WRITE|FM1_GUARD_BUS|FM1_GUARD_PC);
 fm1_audio_stop();fm1_audio_output_enable(0u);lcd_init();fm1_input_init();etch_irq_bridge_init(&native_irq_bridge,fm1_ticks());
 fm1_timer5_start(isr_timer5,4);fm1_irq_enable_all();
 fm1_delay_ms(40);
 /* Felucca's default panel maps OCT- / OCT+ to raw button 0 / 1.
    OCT- at boot bypasses diagnostics and requests ROM UBOOT. */
 if(fm1_in.buttons&1u){etch_boot.pending=0;native_stop();fm1_irq_off();fm1_enter_uboot();}
 /* REC held at boot enters true read-only, silent application Safe Mode. */
 native_recovery_reason=etch_recovery_on_boot(safe,fm1_in.buttons);
 safe=native_recovery_reason!=ETCH_RECOVERY_NORMAL;native_safe=safe;
 uint8_t muted=safe;native_boot_muted=muted;
 if(!safe)etch_boot_splash();
 usb_start();
 etch_editor_init(&native_editor);
 etch_fx_defaults(&native_fx_settings);
 etch_factory_seed(&native_library);
 etch_pattern_clear(&native_pattern);
 for(uint32_t i=0;i<64;i++)native_seqnotes[i]=255;
 etch_project_bind_riffs(&native_project_cfg,native_riff_seeds);
 if(!safe){native_library_boot();
  if(native_store_status==ETCH_STORE_FOREIGN)native_flash_probe=etch_flash_probe_read(native_store_io.read);
  /* Foreign Felucca data is read-only: never auto-erase or boot to SAFE.
   * Use RAM factory patches; explicit manual initialise is required for SAVE. */
  if(native_store_status==ETCH_STORE_FOREIGN){
   etch_factory_seed(&native_library);etch_pattern_clear(&native_pattern);
   for(uint32_t w=0;w<3;w++)for(uint32_t i=0;i<128;i++)
    native_editor.points[w][i]=native_library.active[w][i];
   native_editor.dirty=1;
  }else if(etch_recovery_storage_is_fault(native_store_status)){
   native_recovery_reason=ETCH_RECOVERY_STORAGE_FAULT;native_safe=safe=muted=native_boot_muted=1;
  }
  if(native_store_status==ETCH_STORE_OK || native_store_status==ETCH_STORE_EMPTY){
   if(native_store_status==ETCH_STORE_EMPTY){for(uint32_t w=0;w<3;w++)for(uint32_t i=0;i<128;i++)native_editor.points[w][i]=native_library.active[w][i];native_editor.dirty=1;}
   uint8_t allzero=1;for(uint32_t i=0;i<64;i++)if(native_library.sequence[i])allzero=0;
   if(!allzero)for(uint32_t i=0;i<64;i++)native_seqnotes[i]=native_library.sequence[i];
  }
 }
 /* Manual and fault-triggered SAFE entries are intentional, not unfinished
  * boot attempts. Preserve the fault count but do not turn a Safe Mode
  * power-cycle itself into a new boot-loop strike. */
 if(safe)etch_boot_stable(&etch_boot,1);
 /* Safe Mode never reads a patch or project from damaged/foreign storage. */
 if(!safe)native_project_restore();
 native_morph_a=native_library.selection[0];native_morph_b=native_library.selection[1];
 native_browser_slot=native_library.selection[0];native_browser_target=0;native_morph_focus=0;
 fm1_adc_init();(void)fm1_adc_read(FM1_ADC_MASTER);
 native_controls.gain=16384;native_controls.morph=8192;native_controls.depth=8192;native_mod_ratio=0;
 if(!safe&&native_store_status==ETCH_STORE_OK&&native_library.reserved>=1&&native_library.reserved<=ETCH_PATCH_SLOTS){native_patch_slot=native_library.reserved-1;native_patch_load(0);if(native_project_cfg.source_version==3)native_project_restore();native_restore_project_bpm();}
 native_editor.dirty=1;native_compile_edit();
 native_parameter_publish();
 int32_t pot=fm1_adc_read(FM1_ADC_MASTER);
 if(pot>=0)native_controls.gain=(uint16_t)(pot*32767/1023);
 if(!muted&&etch_recovery_may_run_audio(native_recovery_reason)){fm1_irq_off();native_start();fm1_irq_enable_all();}
 uint32_t last=0,held=0,previous_buttons=fm1_in.buttons,last_seq_notes=fm1_in.notes,last_ui_paint_ms=0;
 uint32_t native_audio_rearm_held=0, native_auto_fault_at=0,last_gain_sample_ms=0;
 uint8_t native_auto_restarts=0, native_last_tripped=0;
 uint8_t disconnected=1;
 for(;;){
  if(!native_updating && !safe){
   native_output_poll();
   /* The UI transition is an independent stage, not the same event as
    * PC6 enable or the first nonzero sample. Quiet LCD/SPI bus while
    * the analog stage is being powered and the DAC fades in. */
   if(!native_screen_gate.ready)
    (void)etch_output_screen_advance(&native_screen_gate,fm1_ticks(),
     (uint8_t)(!native_suspended&&!native_tripped),
     native_output_analog_ready);
  }
  fm1_wdt_feed();
  usb_retry(etch_ms);
  /* MASTER is a performance control, not an LCD refresh event.
   * Its ADC gets polled five times as often as the legacy UI tick;
   * gain smoothing still happens in the DSP once per sample. */
  if(!safe&&!native_updating&&(uint32_t)(etch_ms-last_gain_sample_ms)>=20u){
   last_gain_sample_ms=etch_ms;
   int32_t master_adc=fm1_adc_read(FM1_ADC_MASTER);
   if(master_adc>=0){
    uint16_t next_gain=(uint16_t)((uint32_t)master_adc*32767u/1023u);
    fm1_irq_off();native_controls.gain=next_gain;fm1_irq_enable_all();
   }
  }
  uint8_t gone=!usb.config||usb.suspended;
  if(gone&&!disconnected){fm1_irq_off();midi_in_overflow=1;fm1_irq_enable_all();}
  /* Explicit local recovery from an audio deadline stop. Press HOME+PLAY
   * for 1 second to restart with fresh voices, never flashing anything. */
  if(!muted&&native_tripped && (fm1_in.buttons&((1u<<8)|(1u<<12)))==((1u<<8)|(1u<<12))){
   if(!native_audio_rearm_held)native_audio_rearm_held=etch_ms;
   if((uint32_t)(etch_ms-native_audio_rearm_held)>=1000u){
    fm1_irq_off();native_stop();native_tripped=0;native_start();fm1_irq_enable_all();
    native_audio_rearm_held=0;first_frame=1;
   }
  }else native_audio_rearm_held=0;
  disconnected=gone;
  /* Stage 0 proved input/USB on hardware, but full ETCH DMA can miss a
   * deadline under display/USB load. Restart at most three times after a
   * 1-second cool-off, on the MAIN thread, never in the audio ISR. A true
   * persistent fault stays stopped for diagnosis and manual HOME+PLAY. */
  if(!muted && native_tripped && !native_updating){
   if(!native_auto_fault_at)native_auto_fault_at=etch_ms?etch_ms:1u;
   if(native_auto_restarts<3u && (uint32_t)(etch_ms-native_auto_fault_at)>=1000u){
    fm1_irq_off();native_start();fm1_irq_enable_all();
    native_auto_restarts++;native_auto_fault_at=0;first_frame=1;
   }
  }else native_auto_fault_at=0;
  if(muted||native_tripped){
   fm1_irq_off();native_midi_drain(0);fm1_irq_enable_all();
   if(native_tripped && usb.config)etch_perf_midi_flush(&native_perf_out,native_panel_usb_emit,0);
  }
  if(usb.uboot_req){etch_boot.pending=0;native_stop();usb_detach();fm1_irq_off();fm1_enter_uboot();}
#if ETCH_NATIVE_OTA
  ota_service();
  if(usb.ota_req){
   usb.ota_req=0;
   {
    uint8_t resume=!muted&&!native_tripped;
    native_pause();native_updating=1;fm1_irq_off();native_midi_drain(0);fm1_irq_enable_all();
    int result=ota_session();native_updating=0;
    if(result==-17){resume=0;native_safe=safe=muted=native_boot_muted=1;
     native_recovery_reason=ETCH_RECOVERY_OTA_CLEANUP;
     etch_boot.failed=2;etch_boot.pending=1;native_stop();}
    /* Returning means no reset occurred; retain live waves but clear gates. */
    if(resume){fm1_irq_off();native_start();native_audio.keys=fm1_in.notes&0x7ffffffu;fm1_irq_enable_all();}
   }
   first_frame=1;previous_buttons=fm1_in.buttons;
  }
#endif
  if(etch_ms>=30000 && etch_recovery_audio_healthy(safe,native_tripped,native_updating,native_halves))
   etch_boot_stable(&etch_boot,0);
  if(!native_panel_capture&&(fm1_in.buttons&3u)==3u){if(!held)held=etch_ms;if(etch_ms-held>=5000){etch_boot.pending=0;native_stop();fm1_irq_off();fm1_enter_uboot();}}else held=0;
  /* Separate PLAY oscilloscope cadence from the 100ms input/control tick.
   * Real audio samples, NOT a synthetic waveform; no DSP or ISR writes.
   * Never starve audio under load or run the renderer during OTA. */
  if(!safe&&!native_updating&&!native_tripped&&!native_draw&&
     web_primary_page() && etch_scope_anim_needed(&web_scope_release,
        native_scope_held_note,native_scope_has_signal,web_last_signal)&&
     (uint32_t)(etch_ms-last_ui_paint_ms)>=etch_ui_live_frame_interval(
       native_audio.voice_limit,
       (uint8_t)(native_missed_streak+(native_ui_pressure>=9u?1u:0u)+(native_ui_pressure>=20u?2u:0u)),
       native_tripped)){
   screen();last_ui_paint_ms=etch_ms;
  }
  if(etch_ms-last>=100){
   last=etch_ms;
   if(safe){
    if(first_frame||etch_ms-last_ui_paint_ms>=1800u||fm1_in.buttons!=previous_buttons||fm1_in.notes!=last_seq_notes){screen();last_ui_paint_ms=etch_ms;}
    previous_buttons=fm1_in.buttons;last_seq_notes=fm1_in.notes;continue;
   }
   int32_t preset=fm1_enc_take(6);
   int32_t dm=fm1_enc_take(2),dd=fm1_enc_take(3),up=fm1_enc_take(4),down=fm1_enc_take(5),select=fm1_enc_take(0);
   int32_t m=(int32_t)etch_encoder_pct(native_controls.morph,(native_draw||native_page?0:dm)),d=(int32_t)etch_encoder_pct(native_controls.depth,(native_draw||native_page?0:dd));
   if(m<0)m=0;if(m>32767)m=32767;if(d<0)d=0;if(d>32767)d=32767;
   uint32_t buttons=fm1_in.buttons,previous_buttons_snapshot=previous_buttons;
   uint32_t pressed=buttons&~previous_buttons;previous_buttons=buttons;
   uint8_t notes_changed=(uint8_t)(fm1_in.notes!=last_seq_notes);
   native_seq_entry_mask&=fm1_in.notes;
   if(native_seq_run)native_seq_entry_mask=0u;
   /* Read-only panel instrumentation: explicitly collect the physical
    * ALGORITHM encoder only while testing, never steal it from other pages. */
   int32_t alg=fm1_enc_take(1);
   const int32_t observations[ETCH_DIAG_ENCODERS]={select,alg,dm,dd,up,down,preset};
   etch_diag_panel_record(&native_panel_diag,observations,fm1_in.notes,pressed);
   /* Exclusive diagnostic capture: no shortcuts, patch writes, octave moves,
    * boot shortcuts or ordinary notes while exercising the full panel.
    * REC starts a fresh run. HOME + SAVE exits (no project write). */
   enum etch_panel_action panel_action=etch_panel_capture_action(native_panel_capture,
     (uint8_t)(native_page==ETCH_UI_SYSTEM&&native_system_tab==3),pressed,buttons);
   if(panel_action!=ETCH_PANEL_NORMAL){
    if(panel_action==ETCH_PANEL_BEGIN){
     native_panel_diag=(struct etch_diag_panel){0};
     etch_diag_panel_record(&native_panel_diag,observations,fm1_in.notes,pressed);
     native_panel_capture=1;native_panic=1;
    }else if(panel_action==ETCH_PANEL_END){
     native_panel_capture=0;
    }
    native_project_cfg.tempo=native_tempo;
   fm1_irq_off();native_edit_keys_blocked=(uint8_t)native_panel_capture;fm1_irq_enable_all();
    native_seq_led_refresh();
    if(first_frame||etch_ms-last_ui_paint_ms>=1800u||pressed||buttons!=previous_buttons_snapshot||notes_changed||select||alg||dm||dd||up||down||preset){screen();last_ui_paint_ms=etch_ms;}
    last_seq_notes=fm1_in.notes;continue;
   }
   uint8_t page_before=native_page,draw_before=native_draw;
   /* V1.20: PRESETS rotary is ignored outside the named PRESETS browser.
    * SAVE editors already use K1 (dm) to choose a destination slot. */
   if(preset && !native_draw && native_page==ETCH_UI_PRESETS){
    uint32_t valid=0;
    for(uint8_t k=0;k<ETCH_PATCH_SLOTS;k++)if(native_library.patches[k].valid)valid|=1u<<k;
    uint8_t next=etch_presets_next_valid(native_presets_cursor,preset,valid);
    if(next!=native_presets_cursor){native_patch_change(next);native_presets_cursor=native_patch_slot;}
   }
   if((pressed&(1u<<6)) && native_page!=ETCH_UI_PROJECT_SAVE && native_page!=ETCH_UI_NAME && native_page!=ETCH_UI_PRESETS){
    native_wave_load_armed=0;native_save_confirm=0;native_patch_confirm=0;
    if(!native_draw)native_editor.wave=etch_ui_draw_target(native_page,native_morph_focus,
                                                          native_browser_target,native_editor.wave);
    native_draw=!native_draw;native_page=0;lcd_fill(0,196,240,40,0x0000);
   }
   /* HOME behaves as a release-triggered short press OR 1500ms hold.
    * Holding never first navigates away from the current page; a held chord
    * (including HOME+PLAY recovery) cannot open the browser. */
   enum etch_home_action home_action=etch_home_gesture_poll(&native_home_gesture,
      (uint8_t)((buttons&(1u<<8))!=0u),
      (uint8_t)((buttons&~(1u<<8))!=0u),etch_ms);
   if(home_action==ETCH_HOME_PRESETS && native_page!=ETCH_UI_NAME && native_page!=ETCH_UI_PROJECT_SAVE){
    native_draw=0;native_page=ETCH_UI_PRESETS;
    native_presets_cursor=native_patch_slot;
    native_home_tap_at=0u;
    native_wave_load_armed=0;native_save_confirm=0;native_patch_confirm=0;
    first_frame=1u;
   }else if(home_action==ETCH_HOME_SHORT ||
      (home_action==ETCH_HOME_PRESETS && (native_page==ETCH_UI_NAME || native_page==ETCH_UI_PROJECT_SAVE))){
    if(native_page==ETCH_UI_PRESETS){
     native_page=ETCH_UI_PLAY;native_draw=0;native_home_tap_at=0u;
    }else if(native_page==ETCH_UI_NAME){native_draw=native_save_origin_draw;native_page=0;native_home_tap_at=0u;}
    else if(native_page==ETCH_UI_PROJECT_SAVE){native_page=ETCH_UI_PLAY;native_draw=0;native_home_tap_at=0u;}
    else if(!native_draw && native_page==ETCH_UI_PLAY && native_home_tap_at &&
            (uint32_t)(etch_ms-native_home_tap_at)<500u){
     native_page=ETCH_UI_PROJECTS;native_project_browser_refresh();native_project_browser=0;
     native_home_tap_at=0u;
    }else{native_page=ETCH_UI_PLAY;native_draw=0;native_home_tap_at=etch_ms?etch_ms:1u;}
    native_wave_load_armed=0;native_save_confirm=0;native_patch_confirm=0;
    /* Short HOME navigates only. Voices, transport and audio FX keep playing. */
    native_save_overwrite=0;
    if(page_before!=native_page || draw_before!=native_draw)lcd_fill(0,196,240,40,0x0000);
   }
   uint8_t just_opened_save=0;
   if((pressed&(1u<<9))&&!safe&&native_page!=ETCH_UI_NAME&&native_page!=ETCH_UI_PROJECT_SAVE&&native_page!=ETCH_UI_TOOLS&&
      etch_project_allow_save_shortcut(native_page,buttons) &&
      !(native_page==ETCH_UI_SYSTEM && native_system_tab==1u && native_store_status==ETCH_STORE_FOREIGN)){
    if(native_draw)native_wave_save_start();
    else native_project_save_start();
    just_opened_save=1;
   }
   /* Explicit five-second storage takeover; never run on boot or normal
    * SAVE. Holding FX+SAVE on SYSTEM/STORAGE acknowledges permanent loss of
    * previous FM1 data in the ETCH storage sectors. Safe/OTA modes exclude it. */
   if(native_page==ETCH_UI_SYSTEM && native_system_tab==1u &&
      native_store_status==ETCH_STORE_FOREIGN && !native_safe && !native_updating &&
      (buttons&((1u<<2)|(1u<<9)))==((1u<<2)|(1u<<9))){
     if(!native_init_held)native_init_held=etch_ms?etch_ms:1u;
     if((uint32_t)(etch_ms-native_init_held)>=5000u){
       native_library_initialize(muted);
       native_init_held=0u;
       native_storage_setup_armed=0u;
       native_project_browser_refresh();
     }
   }else native_init_held=0u;
   if(native_page==ETCH_UI_NAME || native_page==ETCH_UI_PROJECT_SAVE){
    if(native_page==ETCH_UI_PROJECT_SAVE&&select){
     if(!native_save_preset_mode)native_preset_save_start();
     else native_project_save_start();
     first_frame=1;select=0;
    }
    /* Dedicated four-knob name editor. SELECT / ALGORITHM do not silently
     * change a character. K1 slot, K2 name position, K3 character, K4 backspace. */
    native_name_pos=etch_save_position(native_name_pos,dd);
    native_name_turn(up);
    etch_save_backspace(native_save_name,&native_name_pos,down);
    if(up||down||dd||dm){native_save_missing_name=0u;native_save_attempted=0u;}
    if(native_page==ETCH_UI_PROJECT_SAVE){
     if(dm){
      if(native_save_preset_mode){
       native_preset_save_slot=(uint8_t)ui_bound((int32_t)native_preset_save_slot+dm,0,31);
       native_save_name_start(native_library.patches[native_preset_save_slot].valid?
                              native_library.patches[native_preset_save_slot].name:0);
      }else native_project_browser=(uint8_t)ui_bound((int32_t)native_project_browser+dm,0,3);
      native_save_overwrite=0;
     }
    }else if(dm){native_slot=(uint8_t)ui_bound((int32_t)native_slot+dm,0,63);native_save_confirm=0;}
    if((pressed&(1u<<9))&&!just_opened_save){
     if(!etch_save_has_name(native_save_name))native_save_missing_name=1u;
     else if(native_page==ETCH_UI_NAME){
      uint8_t existed=(native_library.valid[native_slot>>5]>>(native_slot&31))&1u;
      uint8_t confirmed=native_save_confirm;
      native_save_attempted=1u;
      native_library_save(muted);
      if(native_store_status==ETCH_STORE_OK && (!existed||confirmed) && !native_save_confirm){
       native_page=ETCH_UI_PLAY;native_draw=1;
       native_saved_until=etch_ms+2300u;native_save_notice_pending=1u;native_saved_wave=1u;
      }
     }else{
      uint8_t occupied=native_save_preset_mode?
        native_library.patches[native_preset_save_slot].valid:
        (uint8_t)((native_project_present&(1u<<native_project_browser))!=0);
      if(occupied&&!native_save_overwrite)native_save_overwrite=1;
      else {
       native_save_overwrite=0;native_save_attempted=1u;
       if(native_save_preset_mode)native_patch_save(muted);
       else native_project_browser_save(muted);
       if(native_store_status==ETCH_STORE_OK){
        native_page=ETCH_UI_PLAY;native_draw=0;
        native_saved_until=etch_ms+2300u;native_save_notice_pending=1u;native_saved_wave=0u;
       }
      }
     }
    }
   }else if(native_page==14||native_page==15){
    struct etch_wave_fx_params *f=&native_editor.fx[native_editor.wave];
    uint16_t *controls[4];if(native_page==14){controls[0]=&f->drive;controls[1]=&f->bitcrush;controls[2]=&f->chorus;controls[3]=&f->delay;}
    else{controls[0]=&f->reverb;controls[1]=&f->fold;controls[2]=&f->smooth;controls[3]=&f->mix;}
    int32_t changes[4]={dm,dd,up,down};uint8_t modified=0;
    for(uint8_t k=0;k<4;k++)if(changes[k]){*controls[k]=(uint16_t)etch_encoder_pct(*controls[k],changes[k]);modified=1;}
    if(modified){native_editor.dirty=1;++native_editor.revision;native_fx_bake_confirm=0;native_fx_warning=1;}
    if(pressed&(1u<<9)){
     if(!native_fx_bake_confirm)native_fx_bake_confirm=1;
     else{etch_editor_bake_fx(&native_editor);native_fx_bake_confirm=0;native_fx_warning=0;native_page=0;}
    }
    if(pressed&(1u<<2))native_page=native_page==14?15:9;
    if(pressed&(1u<<6)){native_fx_bake_confirm=0;native_page=0;}
   }else if(native_page==9){
    native_tool=(uint8_t)ui_bound((int32_t)native_tool+dm,0,12);
    native_tool_span=(int16_t)ui_bound(native_tool_span+dd,-127,127);
    if(native_tool==12u){
     native_editor.cursor=(uint8_t)ui_bound((int32_t)native_editor.cursor+up,0,127);
     native_gen_mutate=(uint8_t)ui_bound((int32_t)native_gen_mutate+down,0,100);
     if(pressed&(1u<<9)){
      etch_editor_genetics(&native_editor,(uint8_t)((native_tool_span+127)*100/254),
                         native_gen_mutate,native_editor.cursor);
      native_page=0;native_draw=1u;first_frame=1;
     }
    }else{
     native_editor.cursor=(uint8_t)ui_bound((int32_t)native_editor.cursor+up-down,0,127);
     if(pressed&(1u<<9))etch_editor_tool(&native_editor,native_tool,native_tool_span);
    }
    if(pressed&(1u<<2))native_page=0;
   }else if(!native_draw){
    /* FX opens DRIVE; SELECT cycles the six synthesis effect editors.
     * ADSR remains exclusively under the ENV button. */
    if((pressed&(1u<<2)) && !(native_page==ETCH_UI_SYSTEM && native_system_tab==1u && native_store_status==ETCH_STORE_FOREIGN)){
     if(native_page==ETCH_UI_PREFX_A)native_page=ETCH_UI_PREFX_B;
     else if(native_page==ETCH_UI_PREFX_B)native_page=ETCH_UI_FX_DRIVE;
     else if(etch_ui_is_fx_editor(native_page))native_page=ETCH_UI_PREFX_A;
     else native_page=ETCH_UI_FX_DRIVE;
    }
    if(select&&etch_ui_is_fx_editor(native_page)){
     native_page=etch_ui_fx_select(native_page,select);
     select=0;
    }
    /* SELECT cycles PLAY -> MORPH -> LIBRARY without taking over EDIT/SEQ. */
    if(select&&etch_ui_is_home(native_page)){
     native_page=etch_ui_home_turn(native_page,select>0?1:-1);
     native_wave_load_armed=0;native_wave_load_error=0;
     select=0;
    }
    if(pressed&(1u<<4))native_page=1;
    if(pressed&(1u<<5))native_page=native_page==ETCH_UI_MOD?ETCH_UI_C_FX:
     native_page==ETCH_UI_C_FX?ETCH_UI_EVOLUTION:ETCH_UI_MOD;
    /* Preserve SEL's existing ETCH shortcut outside the PROJECTS browser.
     * The physical GLO key is button 7, not SEL (matrix button 3). */
    if((pressed&(1u<<3)) && native_page!=ETCH_UI_PROJECTS){
     if(etch_ui_seq_context(native_page)){
      if(native_page==ETCH_UI_SEQ)native_page=ETCH_UI_NOTES;
      else if(native_page==ETCH_UI_NOTES)native_page=ETCH_UI_SEQ;
      else if(native_page==ETCH_UI_HARM)native_page=ETCH_UI_BAKE;
      else native_page=ETCH_UI_SEQ;
     }else{
      native_page=etch_ui_glo_route(native_page,native_guide_performance);
      native_guide_performance=(uint8_t)(native_page==ETCH_UI_GUIDE);
     }
    }
    if(alg && etch_ui_seq_context(native_page)){
     native_page=etch_ui_seq_inspector(native_page,alg);
     alg=0;
    }

    if(pressed&(1u<<10)){native_page=etch_ui_arp_next(native_page);if(native_page==22)native_arp_edit_step=0;}
    if(pressed&(1u<<11)){native_guide_performance=0;
     native_seq_clear_armed=0;native_bake_confirm=0;
     native_page=etch_ui_seq_next(native_page);
     if(native_page==20){
      native_hold_source=etch_seq_hold_owner(&native_pattern,native_seq_step,native_hold_pitch);
      if(native_hold_source==0xffffu)native_hold_source=etch_seq_hold_source_cycle(&native_pattern,native_seq_length,native_seq_step,native_hold_pitch,0xffffu,1);
     }
    }
    if(pressed&(1u<<7)){
     /* Real GLO button on the FM1; PRESETS is a rotary encoder. */
     native_wave_load_armed=0;
     native_page=etch_ui_glo_route(native_page,native_guide_performance);
     native_guide_performance=(uint8_t)(native_page==ETCH_UI_GUIDE);
    }
    if((pressed&(1u<<13))&&native_page==ETCH_UI_MOTION){
     if(native_motion_recording){native_motion_recording=0;native_motion_playing=native_project_cfg.motion.length>1u&&native_project_cfg.motion.mask;}
     else {native_motion_playing=0;native_motion_recording=1;etch_motion_clear(&native_project_cfg.motion);native_motion_last_record_ms=etch_ms-100u;}
     first_frame=1;
    }
    if((pressed&(1u<<12))&&native_page==ETCH_UI_MOTION){
     native_motion_recording=0;native_motion_playing=(uint8_t)(!native_motion_playing&&native_project_cfg.motion.length>1u&&native_project_cfg.motion.mask);
     first_frame=1;
    }
    if((pressed&(1u<<12))&&native_page!=ETCH_UI_MOTION){
     native_seq_run=!native_seq_run;
     if(native_seq_run){native_audio.perf_played=0u;native_audio.perf_current_step=0u;}
     if(!native_seq_run)native_seq_recording=0u;
     native_seq_entry_mask=0u;
    }
    /* Web REC: toggle live record, starting playback if stopped. The
     * destructive clear-step action remains on the advanced GRID editor. */
    if((pressed&(1u<<13)) && etch_seq_is_web_page(native_page)){
     native_seq_recording^=1u;
     if(native_seq_recording){
      for(uint16_t st=0;st<ETCH_SEQ_STEPS;st++)native_seq_rec_touched[st]=0u;
      if(!native_seq_run){native_audio.perf_played=0u;native_audio.perf_current_step=0u;}
      native_seq_run=1u;
     }
    }
    if(native_page==25){
     if(native_system_tab==0u&&(pressed&(1u<<13))){native_page=ETCH_UI_MOTION;first_frame=1;}
     if(select)native_system_tab=etch_ui_system_tab(native_system_tab,select>0?1:-1);
     if(native_system_tab==1 && (pressed&(1u<<13))){
      native_page=27;native_project_browser=etch_store_project_selected();
      native_project_armed=0;native_project_browser_refresh();
     }
    }else if(native_page==27){
     uint8_t next=(uint8_t)ui_bound((int32_t)native_project_browser+dm+select,0,4);
     if(next!=native_project_browser)native_project_armed=0;
     native_project_browser=next;
     native_project_char_index=(uint8_t)ui_bound((int32_t)native_project_char_index+dd,0,11);
     if(up && native_project_browser<4u){
      char *name=native_project_names[native_project_browser];
      int32_t c=(unsigned char)name[native_project_char_index];
      if(c<32||c>90)c=' ';
      c+=up;
      while(c>90)c-=59;
      while(c<32)c+=59;
      name[native_project_char_index]=(char)c;
      native_project_armed=0;
     }
     /* Hold REC+SAVE together twice to remove a project. A different key or
      * slot selection cancels the pending destructive operation. */
     if(native_project_browser<4u && etch_project_delete_chord(native_page,buttons,pressed)){
      if(native_project_armed==3){native_project_armed=0;native_project_browser_delete(muted);}
      else native_project_armed=3;
     }else if(pressed&(1u<<ETCH_PROJECT_SEL_BUTTON)){
      /* The actual SEL button (matrix bit 3) is YES only in PROJECTS.
       * REC (bit 13) keeps its separate transport/delete usage. */
      enum etch_project_ui_choice action=etch_project_sel_choice(
          native_page,native_project_browser,native_project_present,pressed);
      native_project_armed=0;
      if(action==ETCH_PROJECT_NEW){native_new_project();native_project_browser_refresh();}
      else if(action==ETCH_PROJECT_OPEN){
       native_project_browser_load(muted);
       if(native_store_status==ETCH_STORE_OK)native_page=ETCH_UI_PLAY;
      }
     }
    }
    else if(native_page==23||native_page==24){
     /* In LIBRARY only K1/K2 are mapped. K3/K4 never scroll or cancel
      * the pending REC->REC waveform load confirmation. */
     uint8_t changed=(uint8_t)(native_page==24 ? (dm||dd) : (dm||dd||up||down));
     if(changed){native_wave_load_armed=0;native_wave_load_error=0;}
     if(native_page==23){
      /* V0.78 intuitive fixed mapping: K1 A, K2 B, K3 morph, K4
       * phase-modulation depth. Wave C has its own MOD WAVE page. */
      /* ALGORITHM still selects A/B/C as the EDIT target. It does not
       * change the meaning of knobs 1-4 on the MORPH screen. */
      /* ALGORITHM is the distinctive ETCH/glitch-depth control. K4 now
       * changes the modulator ratio. Preserve K1/K2/K3 from V0.78. */
      native_morph_load_wave(0u,dm);
      native_morph_load_wave(1u,dd);
      m=etch_encoder_pct(native_controls.morph,up);
      d=etch_encoder_pct(native_controls.depth,alg);
      native_mod_ratio=(uint8_t)ui_bound((int32_t)native_mod_ratio+down,0,11);
     }else{
      /* K3 and K4 intentionally have no function in the Wave Library. */
      struct etch_library_encoder_state library_state=etch_encoder_library_controls(
          native_browser_slot,native_browser_target,dm,dd,up,down);
      native_browser_slot=library_state.slot;
      native_browser_target=library_state.target;
     }
     /* LIBRARY keeps its deliberate double-REC load. MORPH now loads A
      * and B instantly via K1/K2, so it no longer has a duplicate action. */
     if(native_page==24 && (pressed&(1u<<13))){
      uint8_t valid=etch_ui_wave_valid(native_library.valid,native_browser_slot);
      uint8_t action=etch_ui_wave_load_click(&native_wave_load_armed,&native_wave_load_page,native_page,valid);
      if(!action)native_wave_load_error=1;
      else if(action==2){
       uint8_t old_wave=native_editor.wave,old_slot=native_slot;
       native_slot=native_browser_slot;native_editor.wave=native_browser_target;
       native_library_load();native_editor.wave=old_wave;native_slot=old_slot;
       native_morph_a=native_library.selection[0];native_morph_b=native_library.selection[1];
       native_wave_load_armed=0;native_wave_load_error=0;
      }
     }
    }
    else if(native_page==ETCH_UI_PREFX_A||native_page==ETCH_UI_PREFX_B){
     uint8_t wave=native_page==ETCH_UI_PREFX_B;
     native_pre_insert_slot=etch_v117_pre_slot(native_pre_insert_slot,dm);
     etch_v117_pre_turn(&native_fx_settings,wave,native_pre_insert_slot,dd,up,down);
    }
    else if(native_page==ETCH_UI_C_FX){
     etch_c_fx_set_target(&native_fx_settings,(uint8_t)ui_bound((int32_t)etch_c_fx_target(&native_fx_settings)+dm,0,31));
     etch_c_fx_set_depth(&native_fx_settings,(uint16_t)etch_encoder_pct(etch_c_fx_depth(&native_fx_settings),dd));
     /* SELECT changes KEYED/FREE/TEMPO on C FX too. Previously this
      * encoder did nothing here, leaving the RATE knob apparently stuck. */
     if(select)native_mod_mode=(uint8_t)ui_bound((int32_t)native_mod_mode+select,0,2);
     if(native_mod_mode==1){if(up)native_free_millihz=(uint32_t)etch_encoder_hz(native_free_millihz,up);}
     else native_mod_ratio=(uint8_t)ui_bound((int32_t)native_mod_ratio+up,0,11);
     native_morph_load_wave(2u,down);
    }
    else if(native_page==ETCH_UI_FX_DRIVE){
     native_drive=(uint16_t)etch_encoder_pct(native_drive,dm);
     native_filter_type=(uint8_t)ui_bound((int32_t)native_filter_type+dd,0,3);
     native_cutoff=etch_encoder_cutoff(native_cutoff,up);native_filter_type=etch_filter_enable_if_cutoff_edited(native_filter_type,up);
     native_resonance=(uint16_t)etch_encoder_pct(native_resonance,down);
     native_filter_type=etch_filter_enable_if_cutoff_edited(native_filter_type,down);
    }
    else if(native_page==ETCH_UI_FX_CHORUS){
     native_chorus=(uint16_t)etch_encoder_pct(native_chorus,dm);
     native_fx_settings.chorus_rate=(uint16_t)etch_encoder_pct(native_fx_settings.chorus_rate,dd);
     native_fx_settings.chorus_depth=(uint16_t)etch_encoder_pct(native_fx_settings.chorus_depth,up);
     native_fx_settings.routing=(uint8_t)ui_bound((int32_t)native_fx_settings.routing+down,0,1);
    }
    else if(native_page==ETCH_UI_FX_DELAY){
     native_delay=(uint16_t)etch_encoder_pct(native_delay,dm);
     native_fx_settings.delay_time=(uint16_t)etch_encoder_pct(native_fx_settings.delay_time,dd);
     native_fx_settings.delay_feedback=(uint16_t)etch_encoder_pct(native_fx_settings.delay_feedback,up);
     native_fx_settings.routing=(uint8_t)ui_bound((int32_t)native_fx_settings.routing+down,0,1);
    }
    else if(native_page==ETCH_UI_FX_RING){
     native_fx_settings.ring_mix=(uint16_t)etch_encoder_pct(native_fx_settings.ring_mix,dm);
     native_fx_settings.ring_rate=(uint16_t)etch_encoder_pct(native_fx_settings.ring_rate,dd);
     native_fx_settings.ring_shape=(uint16_t)etch_encoder_pct(native_fx_settings.ring_shape,up);
     native_fx_settings.ring_bias=(uint16_t)etch_encoder_pct(native_fx_settings.ring_bias,down);
    }
    else if(native_page==ETCH_UI_FX_EQ){
     native_fx_settings.eq_low=(uint16_t)etch_encoder_pct(native_fx_settings.eq_low,dm);
     native_fx_settings.eq_mid=(uint16_t)etch_encoder_pct(native_fx_settings.eq_mid,dd);
     native_fx_settings.eq_high=(uint16_t)etch_encoder_pct(native_fx_settings.eq_high,up);
     native_fx_settings.eq_hpf=(uint16_t)etch_encoder_pct(native_fx_settings.eq_hpf,down);
    }
    else if(native_page==ETCH_UI_FX_COMP){
     native_fx_settings.comp_threshold=(uint16_t)etch_encoder_pct(native_fx_settings.comp_threshold,dm);
     native_fx_settings.comp_ratio=(uint16_t)etch_encoder_pct(native_fx_settings.comp_ratio,dd);
     native_fx_settings.comp_attack=(uint16_t)etch_encoder_pct(native_fx_settings.comp_attack,up);
     native_fx_settings.comp_release=(uint16_t)etch_encoder_pct(native_fx_settings.comp_release,down);
    }
    else if(native_page==ETCH_UI_FX_TREM){
     native_fx_settings.trem_rate=(uint16_t)etch_encoder_pct(native_fx_settings.trem_rate,dm);
     native_fx_settings.trem_depth=(uint16_t)etch_encoder_pct(native_fx_settings.trem_depth,dd);
     native_fx_settings.trem_shape=(uint16_t)etch_encoder_pct(native_fx_settings.trem_shape,up);
     native_fx_settings.trem_mix=(uint16_t)etch_encoder_pct(native_fx_settings.trem_mix,down);
    }
    else if(native_page==ETCH_UI_FX_PHASER){
     native_fx_settings.phaser_rate=(uint16_t)etch_encoder_pct(native_fx_settings.phaser_rate,dm);
     native_fx_settings.phaser_depth=(uint16_t)etch_encoder_pct(native_fx_settings.phaser_depth,dd);
     native_fx_settings.phaser_feedback=(uint16_t)etch_encoder_pct(native_fx_settings.phaser_feedback,up);
     native_fx_settings.phaser_mix=(uint16_t)etch_encoder_pct(native_fx_settings.phaser_mix,down);
    }
    else if(native_page==ETCH_UI_FX_FLANGER){
     native_fx_settings.flanger_rate=(uint16_t)etch_encoder_pct(native_fx_settings.flanger_rate,dm);
     native_fx_settings.flanger_depth=(uint16_t)etch_encoder_pct(native_fx_settings.flanger_depth,dd);
     native_fx_settings.flanger_feedback=(uint16_t)etch_encoder_pct(native_fx_settings.flanger_feedback,up);
     native_fx_settings.flanger_mix=(uint16_t)etch_encoder_pct(native_fx_settings.flanger_mix,down);
    }
    else if(native_page==ETCH_UI_FX_DECIM){
     native_fx_settings.decim_bits=(uint16_t)etch_encoder_pct(native_fx_settings.decim_bits,dm);
     native_fx_settings.decim_rate=(uint16_t)etch_encoder_pct(native_fx_settings.decim_rate,dd);
     native_fx_settings.decim_drive=(uint16_t)etch_encoder_pct(native_fx_settings.decim_drive,up);
     native_fx_settings.decim_mix=(uint16_t)etch_encoder_pct(native_fx_settings.decim_mix,down);
    }
    else if(native_page==ETCH_UI_FX_FOLDER){
     native_fx_settings.folder_fold=(uint16_t)etch_encoder_pct(native_fx_settings.folder_fold,dm);
     native_fx_settings.folder_symmetry=(uint16_t)etch_encoder_pct(native_fx_settings.folder_symmetry,dd);
     native_fx_settings.folder_tone=(uint16_t)etch_encoder_pct(native_fx_settings.folder_tone,up);
     native_fx_settings.folder_mix=(uint16_t)etch_encoder_pct(native_fx_settings.folder_mix,down);
    }
    else if(native_page==ETCH_UI_FX_TAPE){
     native_fx_settings.tape_wow=(uint16_t)etch_encoder_pct(native_fx_settings.tape_wow,dm);
     native_fx_settings.tape_flutter=(uint16_t)etch_encoder_pct(native_fx_settings.tape_flutter,dd);
     native_fx_settings.tape_age=(uint16_t)etch_encoder_pct(native_fx_settings.tape_age,up);
     native_fx_settings.tape_mix=(uint16_t)etch_encoder_pct(native_fx_settings.tape_mix,down);
    }
    else if(native_page==ETCH_UI_FX_REVERSE){
     native_fx_settings.reverse_time=(uint16_t)etch_encoder_pct(native_fx_settings.reverse_time,dm);
     native_fx_settings.reverse_feed=(uint16_t)etch_encoder_pct(native_fx_settings.reverse_feed,dd);
     native_fx_settings.reverse_tone=(uint16_t)etch_encoder_pct(native_fx_settings.reverse_tone,up);
     native_fx_settings.reverse_mix=(uint16_t)etch_encoder_pct(native_fx_settings.reverse_mix,down);
    }
    else if(native_page==ETCH_UI_FX_VIBRATO){
     native_fx_settings.vibrato_rate=(uint16_t)etch_encoder_pct(native_fx_settings.vibrato_rate,dm);
     native_fx_settings.vibrato_depth=(uint16_t)etch_encoder_pct(native_fx_settings.vibrato_depth,dd);
     native_fx_settings.vibrato_shape=(uint16_t)etch_encoder_pct(native_fx_settings.vibrato_shape,up);
     native_fx_settings.vibrato_mix=(uint16_t)etch_encoder_pct(native_fx_settings.vibrato_mix,down);
    }
    else if(native_page==ETCH_UI_FX_ROTARY){
     native_fx_settings.rotary_rate=(uint16_t)etch_encoder_pct(native_fx_settings.rotary_rate,dm);
     native_fx_settings.rotary_depth=(uint16_t)etch_encoder_pct(native_fx_settings.rotary_depth,dd);
     native_fx_settings.rotary_drive=(uint16_t)etch_encoder_pct(native_fx_settings.rotary_drive,up);
     native_fx_settings.rotary_mix=(uint16_t)etch_encoder_pct(native_fx_settings.rotary_mix,down);
    }
    else if(native_page==ETCH_UI_FX_WAH){
     native_fx_settings.wah_sens=(uint16_t)etch_encoder_pct(native_fx_settings.wah_sens,dm);
     native_fx_settings.wah_attack=(uint16_t)etch_encoder_pct(native_fx_settings.wah_attack,dd);
     native_fx_settings.wah_reso=(uint16_t)etch_encoder_pct(native_fx_settings.wah_reso,up);
     native_fx_settings.wah_mix=(uint16_t)etch_encoder_pct(native_fx_settings.wah_mix,down);
    }
 else if(native_page==ETCH_UI_FX_REVERB){
     native_reverb=(uint16_t)etch_encoder_pct(native_reverb,dm);
     native_fx_settings.reverb_size=(uint16_t)etch_encoder_pct(native_fx_settings.reverb_size,dd);
     native_fx_settings.reverb_decay=(uint16_t)etch_encoder_pct(native_fx_settings.reverb_decay,up);
     native_fx_settings.routing=(uint8_t)ui_bound((int32_t)native_fx_settings.routing+down,0,1);
    }
    else if(native_page==ETCH_UI_ENV){native_env_ms[0]=(uint16_t)etch_encoder_unit(native_env_ms[0],dm,0,10000);native_env_ms[1]=(uint16_t)etch_encoder_unit(native_env_ms[1],dd,0,10000);native_sustain=(uint16_t)etch_encoder_pct(native_sustain,up);native_env_ms[2]=(uint16_t)etch_encoder_unit(native_env_ms[2],down,0,10000);}
    else if(native_page==2){
     /* WAVE C alone: K1 selects audible wave, K2 depth,
      * K3 keyed ratio / FREE rate, K4 PHASE or MORPH destination.
      * SELECT chooses KEYED/FREE/TEMPO. MORPH page stays unchanged. */
     native_morph_load_wave(2u,dm);
     d=etch_encoder_pct(native_controls.depth,dd);
     if(select)native_mod_mode=(uint8_t)ui_bound((int32_t)native_mod_mode+select,0,2);
     if(native_mod_mode==1){if(up)native_free_millihz=(uint32_t)etch_encoder_hz(native_free_millihz,up);}
     else native_mod_ratio=(uint8_t)ui_bound((int32_t)native_mod_ratio+up,0,11);
     native_controls.mod_target=(uint8_t)ui_bound((int32_t)native_controls.mod_target+down,0,1);
    }
    else if(native_page==3){native_filter_type=(uint8_t)ui_bound((int32_t)native_filter_type+dm,0,3);native_cutoff=etch_encoder_cutoff(native_cutoff,dd);native_filter_type=etch_filter_enable_if_cutoff_edited(native_filter_type,dd);native_resonance=(uint16_t)etch_encoder_pct(native_resonance,up);native_filter_type=etch_filter_enable_if_cutoff_edited(native_filter_type,up);native_drive=(uint16_t)etch_encoder_pct(native_drive,down);}
    else if(native_page==4){native_drive=(uint16_t)etch_encoder_pct(native_drive,dm);native_chorus=(uint16_t)etch_encoder_pct(native_chorus,dd);native_delay=(uint16_t)etch_encoder_pct(native_delay,up);native_reverb=(uint16_t)etch_encoder_pct(native_reverb,down);}
    else if(native_page==16){
     native_fx_settings.chorus_rate=(uint16_t)etch_encoder_pct(native_fx_settings.chorus_rate,dm);
     native_fx_settings.chorus_depth=(uint16_t)etch_encoder_pct(native_fx_settings.chorus_depth,dd);
     native_fx_settings.delay_time=(uint16_t)etch_encoder_pct(native_fx_settings.delay_time,up);
     native_fx_settings.delay_feedback=(uint16_t)etch_encoder_pct(native_fx_settings.delay_feedback,down);
    }
    else if(native_page==17){
     native_fx_settings.reverb_size=(uint16_t)etch_encoder_pct(native_fx_settings.reverb_size,dm);
     native_fx_settings.reverb_decay=(uint16_t)etch_encoder_pct(native_fx_settings.reverb_decay,dd);
     native_fx_settings.routing=(uint8_t)ui_bound((int32_t)native_fx_settings.routing+up,0,1);
     native_delay=(uint16_t)etch_encoder_pct(native_delay,down);
    }
    else if(native_page==5){native_arp_mode=(uint8_t)ui_bound((int32_t)native_arp_mode+dm,0,7);
     native_arp_rate=(uint8_t)ui_bound((int32_t)native_arp_rate+dd,0,3);
     native_arp_octaves=etch_arp_octave_turn(native_arp_octaves,up);
     if(native_arp_mode==7u && down){
      int32_t n=down>0?down:-down;if(n>64)n=64;
      fm1_irq_off();
      while(n--)native_riff_seeds[native_patch_slot]=(uint16_t)etch_riff_regenerate(native_riff_seeds[native_patch_slot]);
      fm1_irq_enable_all();
     }else if(native_arp_mode!=7u)native_tempo=(uint8_t)ui_bound((int32_t)native_tempo+down,30,240);}
    else if(native_page==22){
     /* Custom ARP order lives with the selected patch; each step references
      * a held key (across octaves), or 255 for a silent rest. */
     fm1_irq_off();
     struct etch_arp_program *ap=&native_project_cfg.arp_programs[native_patch_slot];
     if(!ap->length)etch_arp_program_init(ap);
     native_arp_edit_step=(uint8_t)ui_bound((int32_t)native_arp_edit_step+dm+select,0,ap->length-1);
     uint8_t *entry=&ap->steps[native_arp_edit_step];
     if(dd)*entry=(uint8_t)ui_bound((int32_t)(*entry==255?0:*entry)+dd,0,31);
     if(up)*entry=up>0?255:(uint8_t)(*entry==255?0:*entry);
     if(down)ap->length=(uint8_t)ui_bound((int32_t)ap->length+down,1,ETCH_ARP_PROGRAM_STEPS);
     if(native_arp_edit_step>=ap->length)native_arp_edit_step=(uint8_t)(ap->length-1);
     if(pressed&(1u<<13))*entry=*entry==255?0:255;
     fm1_irq_enable_all();
    }
    else if(native_page==6||native_page==10||native_page==11||native_page==12||native_page==18||native_page==19||native_page==20||native_page==21||native_page==26){
     fm1_irq_off();
     struct etch_seq_step *st=&native_pattern.steps[native_seq_step];
     if(native_page==6){
      if(dm||dd||up||down||select)native_seq_clear_armed=0;
      if(dm){static const uint16_t lengths[6]={8,16,32,64,128,256};uint8_t pos=0;
       while(pos<5&&lengths[pos]!=native_seq_length)++pos;
       pos=(uint8_t)ui_bound((int32_t)pos+dm,0,5);native_seq_length=lengths[pos];if(native_seq_step>=native_seq_length)native_seq_step=native_seq_length-1;}
      native_seq_rate=(uint8_t)ui_bound((int32_t)native_seq_rate+dd,0,3);
      /* Web controls: K1 LEN, K2 RATE, K3 SWING, K4 GATE.
       * SELECT moves step. BPM is shared with ARP in the CLOCK editor. */
      native_seq_swing=(uint8_t)ui_bound((int32_t)native_seq_swing+up,0,75);
      native_seq_gate=(uint8_t)ui_bound((int32_t)native_seq_gate+down,5,100);
      uint16_t old_step=native_seq_step;
      native_seq_step=(uint16_t)ui_bound((int32_t)native_seq_step+select,0,native_seq_length-1);
      if(select&&native_seq_entry_mask && native_seq_step>=old_step)
       for(uint8_t k=0;k<27u;k++)if((native_seq_entry_mask&fm1_in.notes)&(1u<<k)){
        etch_seq_parity_extend(&native_pattern,native_seq_entry_step[k],
              native_seq_step,native_seq_length,native_seq_entry_pitch[k]);
       }
     }else if(native_page==10){
      native_seq_step=(uint16_t)ui_bound((int32_t)native_seq_step+select,0,native_seq_length-1);
      st=&native_pattern.steps[native_seq_step];
      /* Re-select a valid note after switching to a shorter/empty chord. */
      native_note_sel=etch_seq_note_cursor(st->count,native_note_sel);
      native_note_sel=etch_seq_note_cursor(st->count,(uint8_t)ui_bound((int32_t)native_note_sel+dm,0,st->count?st->count-1:0));
      native_edit_feedback=0;
      if(st->count){struct etch_seq_note n=st->notes[native_note_sel];
       n.velocity=(uint8_t)ui_bound((int32_t)n.velocity+dd,1,127);
       n.length=(uint8_t)ui_bound((int32_t)n.length+up,1,255);
       n.note=(uint8_t)ui_bound((int32_t)n.note+down,0,127);
       if(dd||up||down){uint8_t edited=etch_seq_note_edit(&native_pattern,native_seq_step,native_note_sel,n.note,n.velocity,n.length);
        native_edit_feedback=edited==2?2:edited==1?1:3;
       }
       /* REC is live capture on the simulator STEP page, never delete. To
        * remove a trigger, press its piano key while stopped. Other lanes
        * (HOLD / locks) are protected from accidental edits. */
      }
     }else if(native_page==11){
      native_harmony.root=(uint8_t)ui_bound((int32_t)native_harmony.root+dm,0,11);
      native_harmony.mode=(uint8_t)ui_bound((int32_t)native_harmony.mode+dd,0,8);
      native_harmony.keylock=(uint8_t)ui_bound((int32_t)native_harmony.keylock+up,0,2);
      native_harmony.chord=(uint8_t)ui_bound((int32_t)native_harmony.chord+down,0,2);
     }else if(native_page==12){
      native_lock_sel=(uint8_t)ui_bound((int32_t)native_lock_sel+dm,0,5);
      st->locks[native_lock_sel]=(uint16_t)etch_encoder_lock_value(st->locks[native_lock_sel],native_lock_sel,dd);
      if(up)st->lock_mask=(uint8_t)(up>0?st->lock_mask|(1u<<native_lock_sel):st->lock_mask&~(1u<<native_lock_sel));
      native_seq_step=(uint16_t)ui_bound((int32_t)native_seq_step+select+down,0,native_seq_length-1);
     }
     if(native_page==21){
      if(dm||dd||up||down||select)native_bake_confirm=0;
      native_bake_scope=(uint8_t)ui_bound((int32_t)native_bake_scope+dm,0,1);
      native_seq_step=(uint16_t)ui_bound((int32_t)native_seq_step+select+dd,0,native_seq_length-1);
      native_harmony.chord=(uint8_t)ui_bound((int32_t)native_harmony.chord+up,0,2);
      native_harmony.keylock=(uint8_t)ui_bound((int32_t)native_harmony.keylock+down,0,2);
      if(pressed&(1u<<13)){
       int changed=etch_seq_bake_harmony(&native_pattern,native_seq_length,native_seq_step,native_bake_scope,&native_harmony,0);
       if(changed<0){native_edit_feedback=3;native_bake_confirm=0;}
       else if(!native_bake_confirm)native_bake_confirm=1;
       else{native_edit_feedback=etch_seq_bake_harmony(&native_pattern,native_seq_length,native_seq_step,native_bake_scope,&native_harmony,1)<0?3:1;native_bake_confirm=0;}
      }
     }else if(native_page==18){
      native_seq_step=etch_seq_grid_move(native_seq_step,native_seq_length,dm,dd,up-down,select);
     }else if(native_page==20){
      uint16_t prior_step=native_seq_step;uint8_t prior_pitch=native_hold_pitch;
      native_seq_step=(uint16_t)ui_bound((int32_t)native_seq_step+dm+select+down*8,0,native_seq_length-1);
      native_hold_pitch=(uint8_t)ui_bound((int32_t)native_hold_pitch+dd,0,127);
      if(prior_step!=native_seq_step||prior_pitch!=native_hold_pitch){
       native_hold_source=etch_seq_hold_owner(&native_pattern,native_seq_step,native_hold_pitch);
       if(native_hold_source==0xffffu)native_hold_source=etch_seq_hold_source_cycle(&native_pattern,native_seq_length,native_seq_step,native_hold_pitch,0xffffu,1);
      }
      if(up)native_hold_source=etch_seq_hold_source_cycle(&native_pattern,native_seq_length,native_seq_step,native_hold_pitch,native_hold_source,(int8_t)(up<0?-1:1));
      native_edit_feedback=0;
      if(pressed&(1u<<13)){
       uint8_t present=0;const struct etch_seq_step *hs=&native_pattern.steps[native_seq_step];
       for(uint8_t j=0;j<hs->hold_count;j++)if(hs->holds[j]==native_hold_pitch&&hs->hold_ago[j]&&
        (uint8_t)(native_seq_step-hs->hold_ago[j])==(uint8_t)native_hold_source)present=1;
       if(present)native_edit_feedback=etch_seq_hold_unlink(&native_pattern,native_seq_step,native_hold_pitch,native_hold_source);
       else if(native_hold_source!=0xffffu)native_edit_feedback=etch_seq_hold_link(&native_pattern,native_seq_length,native_seq_step,native_hold_pitch,native_hold_source);
       else native_edit_feedback=3;
      }
     }else if(native_page==26){
      native_harmony.root=etch_ui_fifths_move(native_harmony.root,dm);
      native_harmony.mode=(uint8_t)ui_bound((int32_t)native_harmony.mode+dd,0,8);
      native_harmony.keylock=(uint8_t)ui_bound((int32_t)native_harmony.keylock+up,0,2);
      native_harmony.chord=(uint8_t)ui_bound((int32_t)native_harmony.chord+down,0,2);
     }else if(native_page==19){
      native_harmony.root=(uint8_t)ui_bound((int32_t)native_harmony.root+dm,0,11);
      native_harmony.mode=(uint8_t)ui_bound((int32_t)native_harmony.mode+dd,0,8);
      native_harmony.keylock=(uint8_t)ui_bound((int32_t)native_harmony.keylock+up,0,2);
      native_guide_note=(uint8_t)ui_bound((int32_t)native_guide_note+down,0,127);
      /* SEQ -> GUIDE may still bake chords. GLO -> GUIDE is read-only
       * with respect to the sequencer even when REC or keys are pressed. */
      if(!native_guide_performance && (pressed&(1u<<13)))
       native_edit_feedback=etch_seq_bake_chord(&native_pattern,native_seq_step,native_guide_note,&native_harmony,110,1)?1:3;
     }
     if(native_page==18){
      if(dm||dd||up||down||select)native_seq_clear_armed=0;
      if(pressed&(1u<<13)){
       if(!native_seq_clear_armed||native_seq_clear_step!=native_seq_step){native_seq_clear_armed=1;native_seq_clear_step=native_seq_step;}
       else{native_edit_feedback=etch_seq_step_clear(&native_pattern,native_seq_step,7u)?1:3;native_seq_clear_armed=0;}
      }
     }
     if(native_page==6||native_page==10||native_page==18||native_page==20||native_page==19){
      uint32_t note_pressed=fm1_in.notes&~last_seq_notes;
      if(note_pressed)native_seq_clear_armed=0;
      /* Capture played notes at the actual audio playhead, without any
       * allocator, flash write or UI drawing in the audio ISR. */
      if(native_seq_run && native_seq_recording && note_pressed){
       uint16_t at=etch_seq_actual_step(native_seq_run,native_audio.perf_last_seq,native_audio.perf_played,native_audio.perf_current_step,native_seq_length,native_seq_step);
       if(!native_seq_rec_touched[at]){
        /* Merge with authored notes: live overdub must never destroy
         * HOLD links or existing parameter locks. */
        native_seq_rec_touched[at]=1u;
       }
       for(uint8_t k=0;k<27;k++)if(note_pressed&(1u<<k)){
        int32_t pitch=53+(int32_t)k+12*native_controls.octave;
        if(pitch>=0&&pitch<=127)
         native_edit_feedback=etch_seq_parity_note_exists(&native_pattern,at,(uint8_t)pitch)?1:
           (etch_seq_put(&native_pattern,at,(uint8_t)pitch,110,1)?1:3);
       }
      }
      if(!native_seq_run&&note_pressed)for(uint8_t k=0;k<27;k++)if(note_pressed&(1u<<k)){
       int32_t note=53+(int32_t)k+12*native_controls.octave;
       if(native_page==18){
        if(k<16u){native_seq_step=etch_seq_grid_key_select(native_seq_step,native_seq_length,k);
         native_note_sel=etch_seq_note_cursor(native_pattern.steps[native_seq_step].count,native_note_sel);}
       }else if(note>=0&&note<=127){
        if(native_page==20){native_hold_pitch=(uint8_t)note;
         native_hold_source=etch_seq_hold_owner(&native_pattern,native_seq_step,native_hold_pitch);
         if(native_hold_source==0xffffu)native_hold_source=etch_seq_hold_source_cycle(&native_pattern,native_seq_length,native_seq_step,native_hold_pitch,0xffffu,1);
         native_edit_feedback=1;
        }
        else if(native_page==19){
         native_guide_note=(uint8_t)note;
         if(!native_guide_performance)
          native_edit_feedback=etch_seq_bake_chord(&native_pattern,native_seq_step,(uint8_t)note,&native_harmony,110,1)?1:3;
        }
        else if(native_page==10){
         uint8_t exists=0;
         for(uint8_t n=0;n<st->count;n++)if(st->notes[n].note==(uint8_t)note){
          uint8_t removed=etch_seq_note_remove(&native_pattern,native_seq_step,n);
          exists=1u;native_edit_feedback=removed==1u?1u:3u;break;
         }
         if(!exists)native_edit_feedback=etch_seq_note_key_select(&native_pattern,native_seq_step,(uint8_t)note,&native_note_sel)?1u:3u;
         native_note_sel=etch_seq_note_cursor(st->count,native_note_sel);
        }else {uint8_t rc=etch_seq_toggle(&native_pattern,native_seq_step,(uint8_t)note,127,1);
         if(native_page==ETCH_UI_SEQ && rc==1u){
          native_seq_entry_mask|=1u<<k;
          native_seq_entry_step[k]=native_seq_step;
          native_seq_entry_pitch[k]=(uint8_t)note;
         }
         native_edit_feedback=rc?1:3;}
       }
      }
     }
     fm1_irq_enable_all();
    }
    else if(native_page==13){native_clock_follow=(uint8_t)ui_bound((int32_t)native_clock_follow+dm,0,1);
     native_seq_gate=(uint8_t)ui_bound((int32_t)native_seq_gate+dd,5,100);
     native_seq_swing=(uint8_t)ui_bound((int32_t)native_seq_swing+up,0,75);
     native_tempo=(uint8_t)ui_bound((int32_t)native_tempo+down,30,240);
    }
    else if(native_page==ETCH_UI_EVOLUTION){
     struct etch_evolution_settings *e=&native_project_cfg.evolution;
     /* Four independent physical encoders, with a separately testable
      * mapping. K1 depth / K2 rate / K3 shape / K4 sync. REC toggles depth. */
     etch_v117_evolution_turn(e,dm,dd,up,down,(uint8_t)!!(pressed&(1u<<13)));
    }
    else if(native_page==ETCH_UI_MOTION){m=etch_encoder_pct(native_controls.morph,dm);d=etch_encoder_pct(native_controls.depth,dd);native_cutoff=etch_encoder_cutoff(native_cutoff,up);native_reverb=etch_encoder_pct(native_reverb,down);}
    else if(native_page==7){if(dm){native_patch_change((uint8_t)ui_bound((int32_t)native_patch_slot+dm,0,31));}m=etch_encoder_pct(native_controls.morph,dd);d=etch_encoder_pct(native_controls.depth,up);native_reverb=(uint16_t)etch_encoder_pct(native_reverb,down);}
    else{if(up){native_filter_type=1;native_cutoff=etch_encoder_cutoff(native_cutoff,up);native_filter_type=etch_filter_enable_if_cutoff_edited(native_filter_type,up);}native_reverb=(uint16_t)etch_encoder_pct(native_reverb,down);}
    /* SAVE is handled once, above, and always opens PROJECT SAVE here. */
    /* PATCH button pressed again opens the patch name editor; GLO is SYSTEM. */
   }else{
    /* Wave editor: SEL loads; FX opens tools; GLO opens waveform naming. */
    /* Storage claiming is available only through the visible SYSTEM warning. */
    if(pressed&(1u<<2)){native_page=14;native_fx_bake_confirm=0;}
    else if((pressed&(1u<<7))&&native_store_status!=ETCH_STORE_FOREIGN){native_name_kind=0;native_name_pos=0;native_page=8;}
    else {if(select||dm||dd||up||down)native_save_confirm=0;etch_editor_select(&native_editor,select);
     etch_editor_move_xy32(&native_editor,dm,dd);
     native_harmonic=(uint8_t)ui_bound((int32_t)native_harmonic+up,2,16);
     if(down)etch_editor_add_partial(&native_editor,native_harmonic,down);
     if(pressed&(1u<<4))etch_editor_undo(&native_editor);if(pressed&(1u<<5))etch_editor_redo(&native_editor);
     /* GLO is always SYSTEM; WAVES are loaded through LIBRARY with confirmation. */
     if(pressed&(1u<<3)){
      native_draw=0;native_page=ETCH_UI_GUIDE;native_guide_performance=1;
     }
     /* SAVE is handled once, above, and opens WAVE SAVE from DRAW. */}
   }
   last_seq_notes=fm1_in.notes;
   native_compile_edit();
   int32_t octave=native_controls.octave;
   enum etch_octave_action oct_action=etch_octave_scan(&native_octave_chord,buttons);
   if(oct_action==ETCH_OCT_MINUS)--octave;
   else if(oct_action==ETCH_OCT_PLUS)++octave;
   else if(oct_action==ETCH_OCT_TOGGLE){
    native_mono_mode^=1u;
    /* Audio core retires surplus voices with 128-sample tails. Avoid a
     * hard RESET here: it would click during held-note mode switches. */
    first_frame=1;
   }
   if(octave<-4)octave=-4;if(octave>3)octave=3;
   struct etch_ui_arms ui_arms={native_wave_load_armed,native_save_confirm,
    native_patch_confirm,native_bake_confirm,native_fx_bake_confirm,native_seq_clear_armed};
   if(etch_ui_cancel_on_transition(page_before,native_page,draw_before,native_draw,&ui_arms)){
    native_wave_load_armed=ui_arms.wave_load;native_save_confirm=ui_arms.wave_save;
    native_patch_confirm=ui_arms.patch_save;native_bake_confirm=ui_arms.bake;
    native_fx_bake_confirm=ui_arms.wave_fx_bake;native_seq_clear_armed=ui_arms.seq_clear;
    native_wave_load_error=0;native_edit_feedback=0;
   }
   if(native_motion_recording && native_page==ETCH_UI_MOTION){
    if((uint32_t)(etch_ms-native_motion_last_record_ms)>=100u){
     uint16_t values[4]={(uint16_t)m,(uint16_t)d,native_cutoff,native_reverb};
     uint8_t touched=(uint8_t)((dm?1u:0u)|(dd?2u:0u)|(up?4u:0u)|(down?8u:0u));
     etch_motion_capture(&native_project_cfg.motion,values,touched);
     native_motion_last_record_ms=etch_ms;
     if(native_project_cfg.motion.length==ETCH_MOTION_STEPS){native_motion_recording=0;native_motion_playing=native_project_cfg.motion.mask!=0u;}
    }
   }
   fm1_irq_off();native_edit_keys_blocked=(uint8_t)(native_panel_capture||native_page==18||native_page==20);native_controls.morph=(uint16_t)m;native_controls.depth=(uint16_t)d;native_controls.octave=(int8_t)octave;native_parameter_publish();
   /* HOME must not reset audio voices. A forced etch_native_reset() mid-note
    * jumps the PCM sample to zero, causing the reported audible pop.
    * Explicit panel diagnostics and manual HOME+PLAY recovery remain intact. */
   fm1_irq_enable_all();native_seq_led_refresh();
   if(page_before!=native_page || draw_before!=native_draw)first_frame=1;
   /* Keep 100 ms input/USB service; skip redundant full-region LCD redraws. */
   uint16_t visible_playhead=etch_seq_actual_step(native_seq_run,native_audio.perf_last_seq,native_audio.perf_played,native_audio.perf_current_step,native_seq_length,native_seq_step);
   uint8_t playhead_changed=(uint8_t)(native_seq_run && (etch_seq_is_web_page(native_page)||native_page==ETCH_UI_GRID) && visible_playhead!=last_painted_playhead);
   if(first_frame||pressed||buttons!=previous_buttons_snapshot||oct_action!=ETCH_OCT_NONE||notes_changed||(preset && !native_draw && native_page==ETCH_UI_PRESETS)||select||alg||dm||dd||up||down||(!native_draw&&web_primary_page()&&etch_scope_redraw_on_silence(native_scope_has_signal,web_last_signal))||((!native_draw&&web_primary_page()&&etch_scope_anim_needed(&web_scope_release,native_scope_held_note,native_scope_has_signal,web_last_signal))&&etch_ms-last_ui_paint_ms>=etch_ui_live_frame_interval(native_audio.voice_limit,(uint8_t)(native_missed_streak+(native_ui_pressure>=9u?1u:0u)+(native_ui_pressure>=20u?2u:0u)),native_tripped))||((playhead_changed||native_seq_run||fm1_in.notes)&&etch_ms-last_ui_paint_ms>=((native_audio.voice_limit<8u||native_missed_streak)?750u:(etch_seq_is_web_page(native_page)?40u:(native_page==ETCH_UI_GRID?60u:250u))))||(native_page==25&&native_system_tab>=4&&etch_ms-last_ui_paint_ms>=500u)||native_tripped!=native_last_tripped||page_before!=native_page||draw_before!=native_draw||native_save_notice_pending||(native_saved_until && (int32_t)(etch_ms-native_saved_until)>=0)){
    screen();last_ui_paint_ms=etch_ms;last_painted_playhead=visible_playhead;
   }
   native_last_tripped=native_tripped;
   if(native_saved_until && (int32_t)(etch_ms-native_saved_until)>=0){native_saved_until=0u;native_save_notice_pending=0u;}
  }
 }
}
