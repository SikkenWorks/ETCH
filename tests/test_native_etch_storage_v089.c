/* ETCH 0.89 dedicated persistent storage with no Felucca sample-bank dependencies. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/wave_store.h"
#include "../firmware/fm1-native/native_project_codec.h"
#if !ETCH_STORE_OWNED_ENABLED || !ETCH_P05_MULTI || ETCH_STORE_SPARE_ENABLED
#error "This test requires production FM1 ETCH storage configuration"
#endif
static uint8_t flash[0x100000];
static struct etch_wave_library lib,ret;
static struct etch_native_pattern pattern,readpat;
static struct etch_project_settings cfg,readcfg;
static unsigned writes;
static int rd(uint32_t a,void*p,uint32_t n){assert(n && a+n<=sizeof(flash));memcpy(p,flash+a,n);return 0;}
static int er(uint32_t a){assert(!(a&4095u));assert(a>=ETCH_STORE_OWNED_BASE && a+4096<=ETCH_STORE_OWNED_END);writes++;memset(flash+a,0xff,4096);return 0;}
static int wr(uint32_t a,const void*p,uint32_t n){assert(n && n<=256 && a>=ETCH_STORE_OWNED_BASE && a+n<=ETCH_STORE_OWNED_END);writes++;const uint8_t *src=p;for(uint32_t i=0;i<n;i++){assert((flash[a+i]&src[i])==src[i]);flash[a+i]&=src[i];}return 0;}
static const struct etch_store_io io={rd,er,wr};
static void setup(void){
 memset(flash,255,sizeof(flash));memset(&lib,0,sizeof(lib));memset(&cfg,0,sizeof(cfg));etch_pattern_clear(&pattern);
 memcpy(flash+0x97000,"FELU",4);memcpy(flash+0xA0000,"FSMP",4);memcpy(flash+0xE0000,"OTAX",4);
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;etch_fx_defaults(&cfg.global_fx);
 lib.valid[0]=1;lib.selection[0]=lib.selection[1]=lib.selection[2]=0;
 for(unsigned i=0;i<128;i++)lib.slot[0][i]=(int16_t)(100*i);
 etch_store_project_select(0);etch_store_project_name("ETCH TEST");writes=0;
}
int main(void){
 setup();
 /* Unrelated foreign headers in other areas do NOT lock the empty ETCH bank. */
 assert(etch_store_load_project(&io,&ret,&readpat,&readcfg)==ETCH_STORE_EMPTY);
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)==ETCH_STORE_OK);
 assert(writes>0);
 assert(etch_store_load_project(&io,&ret,&readpat,&readcfg)==ETCH_STORE_OK);
 assert(!memcmp(flash+0x97000,"FELU",4));assert(!memcmp(flash+0xA0000,"FSMP",4));assert(!memcmp(flash+0xE0000,"OTAX",4));
 /* A fresh device with unknown bytes inside ETCH's adopted banks still requires
  * the explicit user-confirmed FORMAT, not a destructive background SAVE. */
 setup();memcpy(flash+ETCH_STORE_OWNED_BASE,"OLD!",4);
 assert(etch_store_load_project(&io,&ret,&readpat,&readcfg)==ETCH_STORE_FOREIGN);
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)==ETCH_STORE_FOREIGN && writes==0);
 assert(etch_store_initialize_project(&io,&lib,&pattern,&cfg,0)!=ETCH_STORE_OK && writes==0);
 assert(etch_store_initialize_project(&io,&lib,&pattern,&cfg,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_OK);
 assert(etch_store_load_project(&io,&ret,&readpat,&readcfg)==ETCH_STORE_OK);
 assert(!memcmp(flash+0x97000,"FELU",4));assert(!memcmp(flash+0xA0000,"FSMP",4));assert(!memcmp(flash+0xE0000,"OTAX",4));
 puts("PASS V0.89: blank ETCH banks save automatically; occupied banks require confirmed format; protected ranges untouched");
}
