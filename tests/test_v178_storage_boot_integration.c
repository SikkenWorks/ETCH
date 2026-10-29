/* SPDX-License-Identifier: GPL-3.0-only
 * Native flash emulator: first-boot modal must never write before YES +
 * completed fresh SAVE hold, or to boot/OTA/presets/other firmware areas.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_storage_boot_prompt.h"
#include "../firmware/fm1-native/native_project_codec.h"
#if !ETCH_STORE_OWNED_ENABLED || !ETCH_P05_MULTI
#error "Requires ETCH_STORE_OWNED_ENABLED=1 ETCH_P05_MULTI=1"
#endif
static uint8_t flash[0x100000],before_nonowned[0x100000-0x28000];
static struct etch_wave_library library,read_library;
static struct etch_native_pattern pattern,read_pattern;
static struct etch_project_settings cfg,read_cfg;
static struct etch_storage_boot_prompt prompt;
static uint32_t writes;static int failure;
static int rd(uint32_t off,void*dst,uint32_t n){
 if(failure)return -1;
 assert(off<=0x100000u&&n<=0x100000u-off);memcpy(dst,flash+off,n);return 0;}
static int er(uint32_t off){
 assert(off>=ETCH_STORE_OWNED_BASE&&off+4096u<=ETCH_STORE_OWNED_END&&!(off&4095u));
 writes++;memset(flash+off,255,4096u);return 0;}
static int wr(uint32_t off,const void*src,uint32_t n){
 assert(off>=ETCH_STORE_OWNED_BASE&&off+n<=ETCH_STORE_OWNED_END&&n<=256u);
 for(uint32_t i=0;i<n;i++){uint8_t b=((const uint8_t*)src)[i];assert((flash[off+i]&b)==b);flash[off+i]&=b;}writes++;return 0;}
static const struct etch_store_io io={rd,er,wr};
#define SEL (1u<<3)
#define HOME (1u<<8)
#define SAVE (1u<<9)
static enum etch_storage_prompt_action step(int d,uint32_t press,uint32_t buttons,uint32_t now){
 return etch_storage_prompt_input(&prompt,d,0,press,buttons,now);
}
int main(void){
 memset(flash,255,sizeof(flash));
 for(uint32_t i=0;i<sizeof(flash);i+=4096u)flash[i]=(uint8_t)(i>>12);
 memcpy(flash+0xA0000u,"FSMP",4u);
 memcpy(flash+ETCH_STORE_OWNED_BASE,"FSMP",4u);
 memcpy(flash+ETCH_STORE_OWNED_BASE+ETCH_STORE_OWNED_BANK_BYTES,"FSMP",4u);
 assert(etch_store_load_project(&io,&read_library,&read_pattern,&read_cfg)==ETCH_STORE_FOREIGN);
 memcpy(before_nonowned,flash,ETCH_STORE_OWNED_BASE);
 memcpy(before_nonowned+ETCH_STORE_OWNED_BASE,flash+ETCH_STORE_OWNED_END,sizeof(flash)-ETCH_STORE_OWNED_END);
 memset(&library,0,sizeof(library));memset(&cfg,0,sizeof(cfg));etch_pattern_clear(&pattern);
 library.valid[0]=1u;library.selection[0]=library.selection[1]=library.selection[2]=0;
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;etch_fx_defaults(&cfg.global_fx);
 etch_store_project_select(0);etch_store_project_name("BOOT OWNED");
 etch_storage_prompt_open(&prompt,ETCH_STORE_FOREIGN,1,1);
 assert(step(0,SEL,0,100)==ETCH_STORAGE_PROMPT_KEEP&&writes==0);
 etch_storage_prompt_open(&prompt,ETCH_STORE_FOREIGN,1,1);
 step(1,0,0,200);step(0,SEL,0,300);
 step(0,0,SAVE,400);step(0,0,SAVE,7000);
 assert(writes==0); /* A pre-held SAVE must never authorize destructive work. */
 step(0,0,0,7100);
 step(0,0,SAVE,7200);
 assert(step(0,0,SAVE,10200)==ETCH_STORAGE_PROMPT_FORMAT&&writes==0);
 assert(etch_store_initialize_project(&io,&library,&pattern,&cfg,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_OK);
 assert(writes>0);
 assert(!memcmp(before_nonowned,flash,ETCH_STORE_OWNED_BASE));
 assert(!memcmp(before_nonowned+ETCH_STORE_OWNED_BASE,
                flash+ETCH_STORE_OWNED_END,sizeof(flash)-ETCH_STORE_OWNED_END));
 assert(etch_store_load_project(&io,&read_library,&read_pattern,&read_cfg)==ETCH_STORE_OK);
 assert(etch_store_initialize_project(&io,&library,&pattern,&cfg,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_INCOMPATIBLE);
 etch_storage_prompt_open(&prompt,ETCH_STORE_OK,1,1);
 assert(prompt.phase==ETCH_STORAGE_PROMPT_CLOSED);
 uint32_t done=writes;
 failure=1;
 etch_storage_prompt_open(&prompt,ETCH_STORE_IO,1,1);
 assert(prompt.phase==ETCH_STORAGE_PROMPT_CLOSED);
 assert(writes==done);
 puts("PASS V1.78 flash integration: no erases before confirmed hold, ETCH-only erase, legacy/OTA preserved, valid ETCH protected");
 return 0;
}
