/* SPDX-License-Identifier: GPL-3.0-only
 * V0.94 owned-region single-project -> V0.95 P05 migration regression.
 * Uses fake flash: NEVER communicates with the actual FM1.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/wave_store.h"
#if !ETCH_P05_MULTI || !ETCH_STORE_OWNED_ENABLED || ETCH_STORE_SPARE_ENABLED || ETCH_P04_EXPANDED
#error "Test production --storage --updater --multi-project layout"
#endif
static uint8_t flash[0x100000];
static struct etch_wave_library lib,loaded;
static struct etch_native_pattern pattern,readpattern;
static struct etch_project_settings cfg,readcfg;
static unsigned writes;static int fail_on=-1;
static int rd(uint32_t a,void *p,uint32_t n){assert(n&&a+n<=sizeof(flash));memcpy(p,flash+a,n);return 0;}
static int er(uint32_t a){assert(!(a&4095u));assert(a>=ETCH_STORE_OWNED_BASE&&a+4096<=ETCH_STORE_OWNED_END);if(fail_on>=0&&(int)writes==fail_on)return -1;writes++;memset(flash+a,0xff,4096);return 0;}
static int wr(uint32_t a,const void *p,uint32_t n){assert(n&&n<=256&&a>=ETCH_STORE_OWNED_BASE&&a+n<=ETCH_STORE_OWNED_END);assert(n+(a&255u)<=256);if(fail_on>=0&&(int)writes==fail_on)return -1;writes++;const uint8_t *s=p;for(unsigned i=0;i<n;i++){assert((flash[a+i]&s[i])==s[i]);flash[a+i]&=s[i];}return 0;}
static const struct etch_store_io io={rd,er,wr};
static void prepare(void){
 memset(flash,0xff,sizeof(flash));memset(&lib,0,sizeof(lib));memset(&cfg,0,sizeof(cfg));
 etch_pattern_clear(&pattern);cfg.tempo=137;cfg.seq_length=16;cfg.seq_gate=75;
 etch_fx_defaults(&cfg.global_fx);lib.selection[0]=lib.selection[1]=lib.selection[2]=0;lib.valid[0]=1;
 memcpy(lib.wave_names[0],"TEST WAVE",9);
 for(int i=0;i<128;i++)lib.slot[0][i]=(int16_t)(i*13-700);
 assert(etch_seq_put(&pattern,0,57,110,2));etch_store_project_select(0);writes=0;fail_on=-1;
 memcpy(flash+0x97000,"FELU",4);memcpy(flash+0xA0000,"FSMP",4);memcpy(flash+0xE0000,"OTAX",4);
}
static void seed_v094(void){
 static uint8_t encoded[ETCH_PROJECT_MAX_BYTES];uint32_t size=0;
 assert(etch_project_encode(&lib,&pattern,&cfg,encoded,sizeof(encoded),&size)==0);
 struct {uint32_t magic,version,generation,bytes,crc,bank,reserved,hcrc;} h={0};
 h.magic=0x38575745u;h.version=9;h.generation=12;h.bytes=size;h.crc=etch_store_crc(encoded,size);h.bank=0;
 h.hcrc=etch_store_crc(&h,28);
 assert(size+256<ETCH_STORE_OWNED_BANK_BYTES);
 memcpy(flash+ETCH_STORE_OWNED_BASE+256,encoded,size);
 memcpy(flash+ETCH_STORE_OWNED_BASE,&h,sizeof(h));
}
static void assert_legacy(void){
 assert(etch_store_load_project_slot(&io,0,&loaded,&readpattern,&readcfg)==ETCH_STORE_OK);
 assert(readcfg.tempo==137);assert(readpattern.steps[0].notes[0].note==57);
 assert(!memcmp(loaded.wave_names[0],"TEST WAVE",9));
 assert(loaded.slot[0][50]==lib.slot[0][50]);
}
int main(void){
 prepare();seed_v094();
 assert(etch_store_load_project(&io,&loaded,&readpattern,&readcfg)==ETCH_STORE_OK);
 assert(readcfg.tempo==137);
 uint8_t present=0;char names[4][12]={{0}};
 assert(etch_store_projects(&io,&present,names)==ETCH_STORE_OK);
 assert(present==1&&memcmp(names[0],"LEGACY",6)==0);
 assert_legacy();
 etch_store_project_select(1);etch_store_project_name("SECOND");
 cfg.tempo=148;etch_pattern_clear(&pattern);assert(etch_seq_put(&pattern,0,65,100,2));
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)==ETCH_STORE_OK);
 assert(etch_store_projects(&io,&present,names)==ETCH_STORE_OK);
 assert(present==3&&!memcmp(names[0],"LEGACY",6)&&!memcmp(names[1],"SECOND",6));
 assert_legacy();
 assert(etch_store_load_project_slot(&io,1,&loaded,&readpattern,&readcfg)==ETCH_STORE_OK);
 assert(readcfg.tempo==148&&readpattern.steps[0].notes[0].note==65);
 /* Simulated reboot: RAM selection resets to zero, committed header must restore project 2. */
 etch_store_project_select(0);
 assert(etch_store_load_project(&io,&loaded,&readpattern,&readcfg)==ETCH_STORE_OK);
 assert(etch_store_project_selected()==1 && readcfg.tempo==148);
 /* Mid-write I/O fault must preserve last complete bank and both projects. */
 etch_store_project_select(1);cfg.tempo=169;writes=0;fail_on=21;
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)!=ETCH_STORE_OK);
 fail_on=-1;
 assert(etch_store_load_project_slot(&io,1,&loaded,&readpattern,&readcfg)==ETCH_STORE_OK);
 assert(readcfg.tempo==148);assert_legacy();
 etch_store_project_select(1);etch_store_project_name("SECOND");
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)==ETCH_STORE_OK);
 assert(etch_store_load_project_slot(&io,1,&loaded,&readpattern,&readcfg)==ETCH_STORE_OK);
 assert(readcfg.tempo==169);assert_legacy();
 /* Save slot 3 and slot 4 without touching unrelated legacy project or flash owners. */
 for(uint8_t i=2;i<4;i++){etch_store_project_select(i);etch_store_project_name(i==2?"THIRD":"FOURTH");
   cfg.tempo=120+i;assert(etch_store_save_project(&io,&lib,&pattern,&cfg)==ETCH_STORE_OK);}
 assert(etch_store_projects(&io,&present,names)==ETCH_STORE_OK&&present==15);
 assert_legacy();
 assert(!memcmp(flash+0x97000,"FELU",4)&&!memcmp(flash+0xA0000,"FSMP",4)&&!memcmp(flash+0xE0000,"OTAX",4));
 puts("PASS V0.95: V0.94 legacy retained, four projects, BPM restore, reboot selection, fault rollback, foreign regions unchanged");
 return 0;
}
