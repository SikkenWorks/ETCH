#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_recovery_policy.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint8_t flash[2*ETCH_STORE_BANK_BYTES];
static struct etch_wave_library source,restored;
static struct etch_native_pattern pat,loaded;
static struct etch_project_settings cfg,cfg_loaded;
static int rd(uint32_t off,void *dst,uint32_t n){
 assert(off>=ETCH_STORE_BASE&&n<=ETCH_STORE_END-off);
 memcpy(dst,flash+off-ETCH_STORE_BASE,n);return 0;
}
static int er(uint32_t off){
 assert(off>=ETCH_STORE_BASE&&off+4096<=ETCH_STORE_END&&!(off&4095));
 memset(flash+off-ETCH_STORE_BASE,255,4096);return 0;
}
static int wr(uint32_t off,const void *src,uint32_t n){
 assert(off>=ETCH_STORE_BASE&&n<=ETCH_STORE_END-off&&n<=256);
 uint8_t *dst=flash+off-ETCH_STORE_BASE;
 for(uint32_t i=0;i<n;i++){assert((dst[i]&((const uint8_t*)src)[i])==((const uint8_t*)src)[i]);dst[i]&=((const uint8_t*)src)[i];}
 return 0;
}
int main(void){
 const struct etch_store_io io={rd,er,wr};
 memset(flash,255,sizeof flash);
 int st=etch_store_load_project(&io,&restored,&loaded,&cfg_loaded);
 assert(st==ETCH_STORE_EMPTY&&!etch_recovery_storage_is_fault(st));
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;
 assert(etch_store_save_project(&io,&source,&pat,&cfg)==ETCH_STORE_OK);
 st=etch_store_load_project(&io,&restored,&loaded,&cfg_loaded);
 assert(st==ETCH_STORE_OK&&!etch_recovery_storage_is_fault(st));
 /* Deliberately corrupt every header: no valid fallback exists. */
 memset(flash,0x4a,sizeof flash);
 st=etch_store_load_project(&io,&restored,&loaded,&cfg_loaded);
 assert(st==ETCH_STORE_FOREIGN&&etch_recovery_storage_is_fault(st));
 enum etch_recovery_reason why=ETCH_RECOVERY_STORAGE_FAULT;
 assert(!etch_recovery_may_write_store(why));
 assert(!etch_recovery_may_run_audio(why));
 assert(etch_recovery_may_enter_ota(why));
 puts("PASS R01 storage: EMPTY/valid boot allowed, corrupt banks latch silent read-only SAFE; updater permitted");
 return 0;
}
