/* SPDX-License-Identifier: GPL-3.0-only
 * Included in bringup.c. Flash operations are explicit, audio-stopped only.
 */
#include "wave_store.h"
#include "native_flash_probe.h"
#include "native_project_codec.h"
#ifndef ETCH_NATIVE_STORAGE
#define ETCH_NATIVE_STORAGE 0
#endif
#if ETCH_STORE_OWNED_ENABLED
#define FL_RANGE_OK(off,n) ((ETCH_NATIVE_STORAGE&&(FL_IN(off,n,ETCH_STORE_BASE,ETCH_STORE_END)||(ETCH_STORE_SPARE_ENABLED&&FL_IN(off,n,ETCH_STORE_SPARE_BASE,ETCH_STORE_SPARE_END))||FL_IN(off,n,ETCH_STORE_OWNED_BASE,ETCH_STORE_OWNED_END)))||(ETCH_NATIVE_OTA&&FL_IN(off,n,0xE0000u,0xE5000u)))
#elif ETCH_P04_EXPANDED
#define FL_RANGE_OK(off,n) ((ETCH_NATIVE_STORAGE&&FL_IN(off,n,ETCH_STORE_EXT_BASE,ETCH_STORE_EXT_END))||(ETCH_NATIVE_OTA&&FL_IN(off,n,0xE0000u,0xE5000u)))
#else
#define FL_RANGE_OK(off,n) ((ETCH_NATIVE_STORAGE&&(FL_IN(off,n,ETCH_STORE_BASE,ETCH_STORE_END)||(ETCH_STORE_SPARE_ENABLED&&FL_IN(off,n,ETCH_STORE_SPARE_BASE,ETCH_STORE_SPARE_END))))||(ETCH_NATIVE_OTA&&FL_IN(off,n,0xE0000u,0xE5000u)))
#endif
#include "hal/fm1_flash.h"
static struct etch_wave_library native_library;
static uint8_t native_slot,native_save_confirm;
static int native_store_status=ETCH_STORE_EMPTY;
/* Last exact PJT10 byte requirement. Updated only by explicit SAVE. */
static uint32_t native_save_required;
static uint8_t native_capacity_checked;
static int native_store_read(uint32_t off,void *dst,uint32_t n){
 if(!FL_IN(off,n,ETCH_STORE_BASE,ETCH_STORE_END) && !FL_IN(off,n,ETCH_STORE_EXT_BASE,ETCH_STORE_EXT_END) && !(ETCH_STORE_SPARE_ENABLED&&FL_IN(off,n,ETCH_STORE_SPARE_BASE,ETCH_STORE_SPARE_END)) && !(ETCH_STORE_OWNED_ENABLED&&FL_IN(off,n,ETCH_STORE_OWNED_BASE,ETCH_STORE_OWNED_END)))return -1;
 uint8_t *p=dst;
 while(n){uint32_t k=n>256?256:n;fm1_irq_off();int rc=FL_FAR(fl_read_ram)(off,p,k);fm1_irq_enable_all();if(rc)return rc;off+=k;p+=k;n-=k;fm1_wdt_feed();}
 return 0;
}
static int native_store_erase(uint32_t off){
 if(!ETCH_NATIVE_STORAGE||!etch_recovery_may_write_store(native_recovery_reason))return -1;
 uint32_t took;fm1_irq_off();int rc=FL_FAR(fl_erase4k_ram)(off,&took);fm1_irq_enable_all();fm1_wdt_feed();return rc;
}
static int native_store_program(uint32_t off,const void *src,uint32_t n){
 if(!ETCH_NATIVE_STORAGE||!etch_recovery_may_write_store(native_recovery_reason))return -1;
 uint32_t took;fm1_irq_off();int rc=FL_FAR(fl_prog_ram)(off,(const uint8_t*)src,n,&took);fm1_irq_enable_all();fm1_wdt_feed();return rc;
}
static const struct etch_store_io native_store_io={native_store_read,native_store_erase,native_store_program};
static void native_library_boot(void){
 if(!ETCH_NATIVE_STORAGE)return;
 native_store_status=etch_store_load_project(&native_store_io,&native_library,&native_pattern,&native_project_cfg);
 if(native_store_status==ETCH_STORE_OK){for(uint32_t w=0;w<3;w++)for(uint32_t i=0;i<128;i++)native_editor.points[w][i]=native_library.active[w][i];native_editor.dirty=1;}
}
static void native_library_load(void){
 uint32_t bit=1u<<(native_slot&31);
 if(!(native_library.valid[native_slot>>5]&bit))return;
 etch_editor_checkpoint(&native_editor);
 for(uint32_t i=0;i<128;i++)native_editor.points[native_editor.wave][i]=native_library.slot[native_slot][i];
 native_editor.has_undo=1;native_editor.dirty=1;++native_editor.revision;native_library.selection[native_editor.wave]=native_slot;
 native_editor.fx[native_editor.wave]=native_project_cfg.slot_fx[native_slot];
 native_project_cfg.wave_fx[native_editor.wave]=native_project_cfg.slot_fx[native_slot];
}
static void native_project_capture(void){
 native_project_cfg.source_version=21;
 native_project_cfg.mono_mode=native_mono_mode;
 native_project_cfg.seq_length=native_seq_length;native_project_cfg.seq_rate=native_seq_rate;
 native_project_cfg.seq_gate=native_seq_gate;native_project_cfg.seq_swing=native_seq_swing;
 native_project_cfg.tempo=native_tempo;native_project_cfg.clock_follow=native_clock_follow;
 native_project_cfg.harmony=native_harmony;native_project_cfg.global_fx=native_fx_settings;
 for(uint32_t i=0;i<3;i++)native_project_cfg.wave_fx[i]=native_editor.fx[i];
}
static void native_library_commit(uint8_t muted){
 if(!ETCH_NATIVE_STORAGE||!etch_recovery_may_write_store(native_recovery_reason)){native_store_status=ETCH_STORE_IO;return;}
 uint8_t tripped=native_tripped;native_pause();
 for(uint32_t w=0;w<3;w++)for(uint32_t i=0;i<128;i++)native_library.active[w][i]=native_editor.points[w][i];
 native_project_capture();
 if(native_library.reserved>=1&&native_library.reserved<=ETCH_PATCH_SLOTS && native_library.patches[native_patch_slot].valid){
  if(!native_cache_pattern(native_patch_slot)){
   native_store_status=ETCH_STORE_FULL;
   if(!muted&&!tripped){fm1_irq_off();native_start();fm1_irq_enable_all();}
   return;
  }
 }
 /* Exact preflight before any flash erase. Keep prior committed snapshot if full. */
 uint32_t required=0;
 int measured=etch_project_measure(&native_library,&native_pattern,&native_project_cfg,&required);
 native_save_required=measured?0:required;native_capacity_checked=(uint8_t)!measured;
 if(!measured&&required>ETCH_PROJECT_MAX_BYTES)native_store_status=ETCH_STORE_FULL;
 else if(measured)native_store_status=ETCH_STORE_VERIFY;
 else native_store_status=etch_store_save_project(&native_store_io,&native_library,&native_pattern,&native_project_cfg);
 if(native_store_status==ETCH_STORE_OK)native_fx_warning=0;
 /* On failure, leave editable RAM untouched so the user can reduce size or retry.
  * The previous committed flash bank remains intact. */
 if(!muted&&!tripped){fm1_irq_off();native_start();native_audio.keys=fm1_in.notes&0x7ffffffu;fm1_irq_enable_all();}
}
static void native_library_save(uint8_t muted){
 if(!ETCH_NATIVE_STORAGE||!etch_recovery_may_write_store(native_recovery_reason)){native_store_status=ETCH_STORE_IO;return;}
 uint32_t bit=1u<<(native_slot&31);
 if((native_library.valid[native_slot>>5]&bit)&&!native_save_confirm){native_save_confirm=1;return;}
 native_save_confirm=0;
 /* The waveform name and the master-library marker must be part of the
  * SAME verified project commit as its samples. Do not mark ordinary project
  * SAVE operations as global-wave updates. */
 for(uint32_t i=0;i<128;i++)native_library.slot[native_slot][i]=native_editor.points[native_editor.wave][i];
 for(uint32_t i=0;i<12;i++)native_library.wave_names[native_slot][i]=native_save_name[i];
 native_library.valid[native_slot>>5]|=bit;native_library.selection[native_editor.wave]=native_slot;
 native_project_cfg.slot_fx[native_slot]=native_editor.fx[native_editor.wave];
 etch_store_global_wave_save();
 native_library_commit(muted);
 if(native_store_status!=ETCH_STORE_OK)etch_store_global_wave_cancel();
}

static void native_library_initialize(uint8_t muted){
 if(!ETCH_NATIVE_STORAGE||!etch_recovery_may_write_store(native_recovery_reason)||native_store_status!=ETCH_STORE_FOREIGN)return;
 uint8_t tripped=native_tripped;native_pause();
 /* Reclaiming old storage must not leave all usable factory wave slots
  * blank. Make a new ETCH project from the known-good factory bank. */
 etch_factory_seed(&native_library);
 for(uint32_t w=0;w<3;w++)for(uint32_t i=0;i<128;i++)native_library.active[w][i]=native_editor.points[w][i];
 /* CLEAR must not carry corrupted/foreign serialized slot and patch FX forward. */
 {uint8_t *b=(uint8_t*)&native_project_cfg;for(uint32_t i=0;i<sizeof(native_project_cfg);i++)b[i]=0;}
 for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)native_riff_seeds[i]=0;
 native_project_capture();
 native_store_status=etch_store_initialize_project(&native_store_io,&native_library,&native_pattern,&native_project_cfg,ETCH_STORE_ERASE_CONFIRM);
 native_save_confirm=0;
 if(!muted&&!tripped){fm1_irq_off();native_start();native_audio.keys=fm1_in.notes&0x7ffffffu;fm1_irq_enable_all();}
}
