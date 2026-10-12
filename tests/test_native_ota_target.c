#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define ETCH_NATIVE_OTA T_OTA
#define OTA_AREA 0xE0000u
#define OTA_AREA_LEN 0x5000u
#define FL_IN(off,n,lo,hi) ((uint32_t)(off)>=(lo)&&(uint32_t)(off)<=(hi)&&(uint32_t)(n)<=(hi)-(uint32_t)(off))
#define FL_FAR(fn) fn
static struct {uint8_t uboot_req;} usb;
static struct {uint32_t buttons;} fm1_in;
static struct {uint32_t pending,failed;} etch_boot;
static uint8_t nor[0x100000];
static uint32_t ticks,irq_masked,erases,pages,reads,feeds,drains,reset;
static const uint8_t *source;
static uint32_t source_len;
static uint32_t fm1_ticks(void){return ticks;}
static void fm1_irq_off(void){assert(!irq_masked);irq_masked=1;}
static void fm1_irq_enable_all(void){assert(irq_masked);irq_masked=0;}
static void fm1_wdt_feed(void){assert(!irq_masked);feeds++;}
static void native_midi_drain(uint8_t play){assert(!play&&irq_masked);drains++;}
static int fl_erase4k_ram(uint32_t off,uint32_t *took){assert(irq_masked&&off>=OTA_AREA&&off+4096<=OTA_AREA+OTA_AREA_LEN&&!(off&4095));erases++;memset(nor+off,255,4096);*took=1;return 0;}
static int fl_prog_ram(uint32_t off,const uint8_t *p,uint32_t n,uint32_t *took){
 assert(irq_masked&&n&&n<=256&&(off&255)+n<=256&&off>=OTA_AREA&&off+n<=OTA_AREA+OTA_AREA_LEN);
 assert(!source||((uintptr_t)p<(uintptr_t)source||(uintptr_t)p>=(uintptr_t)source+source_len)); /* page was copied to RAM */
 for(uint32_t i=0;i<n;i++)nor[off+i]&=p[i];pages++;*took=1;return 0;
}
static int fl_read_ram(uint32_t off,uint8_t *p,uint32_t n){assert(irq_masked&&n<=256&&off+n<=sizeof nor);reads++;memcpy(p,nor+off,n);return 0;}
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c){(void)x;(void)y;(void)w;(void)h;(void)c;}
static void label(const char *p,uint32_t x,uint32_t y,uint16_t c){(void)p;(void)x;(void)y;(void)c;}
static void text_etch(void){}
static uint16_t etch_scope_canvas[240u*116u];
static uint32_t matrix_blits;
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *source){
 assert(!irq_masked && source==etch_scope_canvas && x==0u && y<240u && w==240u && h==40u);
 matrix_blits++;
}
static void lcd_sync(void){}
static void usb_detach(void){}
static void fm1_delay_ms(uint32_t ms){ticks+=ms*24000u;}
static void fm1_enter_update(const uint8_t *p){assert(p);reset++;}
#include "../firmware/fm1-native/ota_target.c"
int main(void){
 uint8_t data[513],out[513];for(unsigned i=0;i<sizeof data;i++)data[i]=(uint8_t)i;memset(nor,255,sizeof nor);source=data;source_len=sizeof data;
 assert(ota_prog(OTA_AREA,data,1)<0&&ota_erase(OTA_AREA)<0&&ota_fread(0,out,1)<0);native_updating=1;
 assert(ota_erase(0)<0&&ota_erase(OTA_AREA+1)<0&&ota_erase(OTA_AREA+OTA_AREA_LEN)<0);
 assert(ota_prog(0,data,1)<0&&ota_prog(OTA_AREA+OTA_AREA_LEN-1,data,2)<0&&ota_prog(0xfffffff0u,data,32)<0);
 assert(ota_fread(0x4000,out,1)<0&&ota_fread(0x97000,out,1)<0&&ota_fread(OTA_AREA+OTA_AREA_LEN,out,1)<0);
 if(T_OTA){assert(!ota_prog(OTA_AREA+241,data,sizeof data)&&pages==3);assert(!ota_fread(OTA_AREA+241,out,sizeof out)&&reads==3&&!memcmp(data,out,sizeof data));assert(!ota_erase(OTA_AREA)&&erases==1);ota_idle();assert(drains==1&&feeds>=7);ota_show(2,0);assert(matrix_blits==6);etch_boot.pending=etch_boot.failed=2;ota_commit(data);assert(reset==1&&!etch_boot.pending&&!etch_boot.failed);}
 else{assert(ota_prog(OTA_AREA,data,1)<0&&ota_erase(OTA_AREA)<0&&ota_fread(0,out,1)<0);ota_commit(data);assert(!reset&&!pages&&!erases&&!reads);}
 ticks=0xffffff00u;ota_clock_ticks=ticks;ota_clock_rem=0;ota_clock_ms=0;ticks+=48000;assert(ota_now_ms()==2);
 usb.uboot_req=1;assert(ota_aborted());usb.uboot_req=0;fm1_in.buttons=3;assert(ota_aborted());fm1_in.buttons=0;assert(!ota_aborted());
 printf("OTA target: build gate %d, stopped-audio guard, ranges, RAM page copies, chunking, timer wrap and cancel passed\n",T_OTA);
}
