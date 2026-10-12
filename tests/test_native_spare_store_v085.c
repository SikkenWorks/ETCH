/* V0.85: blank spare-store adoption must never mutate Felucca sample sectors. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/wave_store.h"
#include "../firmware/fm1-native/native_project_codec.h"
#if !ETCH_STORE_SPARE_ENABLED || !ETCH_P05_MULTI
#error "Compile with -DETCH_STORE_SPARE_ENABLED=1 -DETCH_P05_MULTI=1"
#endif
#define FLASH_SIZE 0x100000u
static uint8_t flash[FLASH_SIZE], before[ETCH_STORE_END-ETCH_STORE_BASE];
static struct etch_wave_library lib,got;
static struct etch_native_pattern pattern,readpat;
static struct etch_project_settings cfg,readcfg;
static unsigned mutated_legacy,mutated_spare,attempts;
static int fail_at=-1;
static int read_cb(uint32_t off,void *dst,uint32_t n){assert(n&&off<=FLASH_SIZE-n);memcpy(dst,flash+off,n);return 0;}
static int erase_cb(uint32_t off){
 assert(off%4096u==0u&&off>=ETCH_STORE_SPARE_BASE&&off+4096u<=ETCH_STORE_SPARE_END);
 if((int)attempts++==fail_at)return -1;
 memset(flash+off,255,4096u);mutated_spare++;return 0;
}
static int write_cb(uint32_t off,const void *src,uint32_t n){
 assert(n&&n<=256u&&(off&255u)+n<=256u&&off>=ETCH_STORE_SPARE_BASE&&off+n<=ETCH_STORE_SPARE_END);
 if((int)attempts++==fail_at)return -1;
 const uint8_t *p=src;for(uint32_t k=0;k<n;k++){assert((flash[off+k]&p[k])==p[k]);flash[off+k]&=p[k];}
 mutated_spare++;return 0;
}
static const struct etch_store_io io={read_cb,erase_cb,write_cb};
static void setup(void){
 memset(flash,255,sizeof(flash));memset(&lib,0,sizeof(lib));memset(&cfg,0,sizeof(cfg));
 etch_pattern_clear(&pattern);cfg.seq_length=16;cfg.tempo=120;cfg.seq_gate=75;
 etch_fx_defaults(&cfg.global_fx);lib.valid[0]=1;
 lib.selection[0]=lib.selection[1]=lib.selection[2]=0;
 for(int i=0;i<128;i++)lib.slot[0][i]=(int16_t)(i*7-300);
 for(uint32_t off=ETCH_STORE_BASE;off<ETCH_STORE_END;off+=4096u){
  flash[off]=0x50;flash[off+3]=(uint8_t)(off>>12);
 }
 memcpy(flash+0xA0000u,"ABCD",4);memcpy(before,flash+ETCH_STORE_BASE,sizeof(before));
 mutated_legacy=mutated_spare=attempts=0;fail_at=-1;etch_store_project_select(0);
}
static void make_note(uint8_t pitch){
 etch_pattern_clear(&pattern);assert(etch_seq_put(&pattern,0,pitch,110,2));
}
static void save_slot(uint8_t slot,uint8_t pitch){
 etch_store_project_select(slot);etch_store_project_name(slot==2?"SPARE THIRD":"SPARE FIRST");
 make_note(pitch);int rc=etch_store_save_project(&io,&lib,&pattern,&cfg);
 if(rc!=ETCH_STORE_OK)printf("save rc=%d at slot %u\n",rc,slot);
 assert(rc==ETCH_STORE_OK);
}
static void check(uint8_t slot,uint8_t pitch){
 int rc=etch_store_load_project_slot(&io,slot,&got,&readpat,&readcfg);
 if(rc!=ETCH_STORE_OK)printf("load rc=%d at slot %u\n",rc,slot);
 assert(rc==ETCH_STORE_OK);
 assert(readpat.steps[0].notes[0].note==pitch);
}
int main(void){
 setup();assert(etch_store_load_project(&io,&got,&readpat,&readcfg)==ETCH_STORE_EMPTY);
 assert(etch_store_active_region()==2u);
 save_slot(2,76);assert(etch_store_active_region()==2u);check(2,76);
 uint8_t present=0;char labels[4][12]={{0}};
 assert(etch_store_projects(&io,&present,labels)==ETCH_STORE_OK);
 assert(present==4u);assert(!memcmp(labels[2],"SPARE THIRD",11));
 save_slot(0,52);check(2,76);check(0,52);
 assert(!memcmp(flash+ETCH_STORE_BASE,before,sizeof(before)));
 assert(mutated_legacy==0&&mutated_spare>0);
 /* Both spare banks occupied by unrecognized data: no write, no erasure. */
 setup();flash[ETCH_STORE_SPARE_BASE]=0x22;uint8_t clone=flash[ETCH_STORE_SPARE_BASE];
 make_note(52);assert(etch_store_save_project(&io,&lib,&pattern,&cfg)==ETCH_STORE_FOREIGN);
 assert(mutated_spare==0&&mutated_legacy==0&&flash[ETCH_STORE_SPARE_BASE]==clone);
 /* Known Felucca FSMP sample owner MUST block automatic spare adoption. */
 setup();memcpy(flash+0xA0000u,"FSMP",4);
 make_note(52);assert(etch_store_save_project(&io,&lib,&pattern,&cfg)==ETCH_STORE_FOREIGN);
 assert(mutated_spare==0&&mutated_legacy==0);
 /* Failed write after first valid commit leaves last valid bank. */
 setup();save_slot(0,60);make_note(90);etch_store_project_select(0);attempts=0;fail_at=10;
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)!=ETCH_STORE_OK);
 fail_at=-1;check(0,60);
 assert(!memcmp(flash+ETCH_STORE_BASE,before,sizeof(before)));
 puts("PASS spare auto-claim with read-only blank scan, legacy Felucca data byte-identical");
 puts("PASS project 3 + project 1 names/notes reload, foreign spare not touched, interrupted write rollback");
}
