/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH target adapter for the GPL Felucca MIDI update protocol.
 * Included after ota.c; all access is main-loop-only, with DMA stopped.
 */
#include "native_ota_matrix.h"
static uint8_t native_updating;
static uint8_t ota_matrix_band_index,ota_matrix_step;
static uint32_t ota_matrix_frame,ota_matrix_last;
static int32_t ota_matrix_error;
static void ota_matrix_paint_band(uint8_t band){
 /* Scope's DMA canvas is safely reused: synthesis and audio DMA were
  * suspended before entering the Felucca-compatible OTA session. */
 uint16_t y=(uint16_t)band*ETCH_MATRIX_BAND_HEIGHT;
 etch_matrix_band(etch_scope_canvas,y,ETCH_MATRIX_BAND_HEIGHT,
                  ota_matrix_frame,ota_matrix_step,ota_matrix_error);
 lcd_blit(0u,y,240u,ETCH_MATRIX_BAND_HEIGHT,etch_scope_canvas);
 lcd_sync();
}

static uint32_t ota_clock_ticks,ota_clock_rem,ota_clock_ms;
static uint32_t ota_now_ms(void){
 uint32_t now=fm1_ticks(),delta=now-ota_clock_ticks;ota_clock_ticks=now;
 ota_clock_ms+=delta/24000u;ota_clock_rem+=delta%24000u;
 if(ota_clock_rem>=24000u){ota_clock_rem-=24000u;++ota_clock_ms;}return ota_clock_ms;
}
static int ota_aborted(void){return usb.uboot_req||(fm1_in.buttons&3u)==3u;}
static void ota_idle(void){
 fm1_wdt_feed();
 if(native_updating){
  fm1_irq_off();native_midi_drain(0);fm1_irq_enable_all();
  /* One 40-line LCD stripe per 120ms keeps USB/flash service dominant.
   * The *separate* ROM/UBOOT loader owns the LCD after ota_commit(). */
  uint32_t now=ota_now_ms();
  if((uint32_t)(now-ota_matrix_last)>=120u){
   ota_matrix_last=now;
   ota_matrix_paint_band(ota_matrix_band_index);
   if(++ota_matrix_band_index>=6u){ota_matrix_band_index=0u;++ota_matrix_frame;}
  }
 }
}
static int ota_erase(uint32_t off){
 if(!ETCH_NATIVE_OTA||!native_updating||!FL_IN(off,0x1000u,OTA_AREA,OTA_AREA+OTA_AREA_LEN)||(off&0xfffu))return -8;
 uint32_t took;fm1_irq_off();int rc=FL_FAR(fl_erase4k_ram)(off,&took);fm1_irq_enable_all();fm1_wdt_feed();return rc;
}
static int ota_prog(uint32_t off,const void *src,uint32_t n){
 if(!ETCH_NATIVE_OTA||!native_updating||!FL_IN(off,n,OTA_AREA,OTA_AREA+OTA_AREA_LEN))return -8;
 const uint8_t *p=src;uint8_t page[256];
 while(n){uint32_t k=256u-(off&255u);if(k>n)k=n;uint32_t took;
  for(uint32_t i=0;i<k;i++)page[i]=p[i]; /* Constants may reside in XIP. */
  fm1_irq_off();int rc=FL_FAR(fl_prog_ram)(off,page,k,&took);fm1_irq_enable_all();fm1_wdt_feed();
  if(rc)return rc;off+=k;p+=k;n-=k;
 }return 0;
}
static int ota_fread(uint32_t off,void *dst,uint32_t n){
 /* Only raw boot comparison and reserved staging reads, never arbitrary addresses. */
 if(!ETCH_NATIVE_OTA||!native_updating||!(FL_IN(off,n,0,0x4000u)||FL_IN(off,n,OTA_AREA,OTA_AREA+OTA_AREA_LEN)))return -8;
 uint8_t *p=dst;
 while(n){uint32_t k=n>256u?256u:n;fm1_irq_off();int rc=FL_FAR(fl_read_ram)(off,p,k);fm1_irq_enable_all();fm1_wdt_feed();if(rc)return rc;off+=k;p+=k;n-=k;}
 return 0;
}
static void ota_show(uint32_t step,int32_t code){
 ota_matrix_step=(uint8_t)step;ota_matrix_error=code;
 /* Initial full-screen fill. Subsequent changes are rate-limited stripes. */
 for(uint8_t b=0u;b<6u;b++)ota_matrix_paint_band(b);
 ota_matrix_last=ota_now_ms();
 if(step>=9u){uint32_t start=ota_now_ms();while(ota_now_ms()-start<1500u)ota_idle();}
}
static void ota_commit(const uint8_t *parm){
 if(!ETCH_NATIVE_OTA||!native_updating)return;
 etch_boot.pending=etch_boot.failed=0;usb_detach();fm1_delay_ms(30);fm1_enter_update(parm);
}
