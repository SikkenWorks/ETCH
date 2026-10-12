/* V0.86: prove explicit Felucca sample2+sample3 claim, no legacy/user-data writes,
 * multi-project CRC streaming, interrupted write rollback and full-byte probe. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../firmware/fm1-native/wave_store.h"
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_flash_probe.h"
#if !ETCH_STORE_OWNED_ENABLED || !ETCH_P05_MULTI
#error "Compile with ETCH_STORE_OWNED_ENABLED=1 and ETCH_P05_MULTI=1"
#endif
static uint8_t flash[0x100000],non_owned_before[ETCH_STORE_OWNED_BASE-ETCH_STORE_BASE];
static struct etch_wave_library lib,readlib;
static struct etch_native_pattern pattern,readpattern;
static struct etch_project_settings cfg,readcfg;
static int writes,fail_at=-1;
static int rd(uint32_t addr,void *dst,uint32_t n){assert(n&&addr+n<=sizeof(flash));memcpy(dst,flash+addr,n);return 0;}
static int er(uint32_t addr){assert(!(addr&4095u));assert(addr>=ETCH_STORE_OWNED_BASE&&addr+4096u<=ETCH_STORE_OWNED_END);if(writes++==fail_at)return -1;memset(flash+addr,255,4096u);return 0;}
static int wr(uint32_t addr,const void *src,uint32_t n){assert(n&&n<=256u&&!(addr>>20)&&(addr&255u)+n<=256u);assert(addr>=ETCH_STORE_OWNED_BASE&&addr+n<=ETCH_STORE_OWNED_END);if(writes++==fail_at)return -1;const uint8_t*p=src;for(uint32_t i=0;i<n;i++){assert((flash[addr+i]&p[i])==p[i]);flash[addr+i]&=p[i];}return 0;}
static const struct etch_store_io io={rd,er,wr};
static void initialize_fixture(void){
 memset(flash,255,sizeof(flash));memset(&lib,0,sizeof(lib));memset(&cfg,0,sizeof(cfg));
 etch_pattern_clear(&pattern);lib.valid[0]=1u;
 lib.selection[0]=lib.selection[1]=lib.selection[2]=0u;
 for(uint32_t j=0;j<128;j++)lib.slot[0][j]=(int16_t)(j*3);
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;etch_fx_defaults(&cfg.global_fx);
 /* Felucca project, sample 1, samples 2+3, protected presets, and unrelated settings. */
 memcpy(flash+0x97000u,"FELU",4);memcpy(flash+0xA0000u,"FSMP",4);
 memcpy(flash+0xB4000u,"FSMP",4);memcpy(flash+0xC8000u,"FSMP",4);
 flash[0x98000u+124u]=0x42u;flash[0xDC000u]=0x18u;flash[0xFC000u]=0x33u;
 memcpy(non_owned_before,flash+ETCH_STORE_BASE,sizeof(non_owned_before));
 writes=0;fail_at=-1;etch_store_project_select(0);etch_store_project_name("WAVE BASE");
}
static void make_note(uint8_t note){etch_pattern_clear(&pattern);assert(etch_seq_put(&pattern,0,note,90,2));}
static void store(uint8_t slot,uint8_t note){etch_store_project_select(slot);etch_store_project_name("ETCH SONG");make_note(note);int rc=etch_store_save_project(&io,&lib,&pattern,&cfg);if(rc)printf("store slot %u err %d\n",slot,rc);assert(rc==ETCH_STORE_OK);}
static void check(uint8_t slot,uint8_t note){assert(etch_store_load_project_slot(&io,slot,&readlib,&readpattern,&readcfg)==ETCH_STORE_OK);assert(readpattern.steps[0].notes[0].note==note);}
int main(void){
 initialize_fixture();
 struct etch_flash_probe fp=etch_flash_probe_read(rd);
 assert(!fp.io_error&&fp.felucca_sample&&fp.sample2_sectors&&fp.sample3_sectors);
 assert(fp.legacy_sectors>=2); /* full read catches marker at offset 124 */
 make_note(60);
 assert(etch_store_load_project(&io,&readlib,&readpattern,&readcfg)==ETCH_STORE_FOREIGN);
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)==ETCH_STORE_FOREIGN&&writes==0);
 assert(etch_store_initialize_project(&io,&lib,&pattern,&cfg,0u)==ETCH_STORE_FOREIGN&&writes==0);
 assert(etch_store_initialize_project(&io,&lib,&pattern,&cfg,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_OK);
 assert(etch_store_active_region()==3);
 assert(memcmp(flash+ETCH_STORE_BASE,non_owned_before,sizeof(non_owned_before))==0);
 assert(flash[0xDC000u]==0x18u&&flash[0xFC000u]==0x33u);
 check(0,60);
 for(uint8_t slot=1;slot<4;slot++)store(slot,(uint8_t)(60u+slot));
 for(uint8_t slot=0;slot<4;slot++)check(slot,(uint8_t)(60u+slot));
 uint8_t mask=0;char names[4][12]={{0}};
 assert(etch_store_projects(&io,&mask,names)==ETCH_STORE_OK&&mask==15u);
 assert(!memcmp(names[3],"ETCH SONG",9));
 assert(etch_store_initialize_project(&io,&lib,&pattern,&cfg,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_INCOMPATIBLE);
 /* Four distinct content-heavy projects must exceed the fixed 20-KiB RAM
  * staging buffer IN TOTAL without overflowing it or truncating the CRC. */
 lib.valid[0]=0x03ffffffu;
 for(uint32_t wave=0;wave<26u;wave++)for(uint32_t j=0;j<128u;j++)
  lib.slot[wave][j]=(int16_t)((wave*4311u+j*19937u)^(j*379u));
 for(uint8_t slot=0;slot<4;slot++)store(slot,(uint8_t)(60u+slot));
 for(uint8_t slot=0;slot<4;slot++)check(slot,(uint8_t)(60u+slot));
 uint32_t largest=0u;
 for(uint32_t bank=0;bank<2u;bank++){
  uint32_t offset=ETCH_STORE_OWNED_BASE+bank*ETCH_STORE_OWNED_BANK_BYTES;
  uint32_t magic=0u,len=0u;
  memcpy(&magic,flash+offset,4u);memcpy(&len,flash+offset+12u,4u);
  if(magic==0x38575745u&&len>largest)largest=len;
 }
 assert(largest>ETCH_PROJECT_MAX_BYTES);
 /* Interrupted overwrite retains the prior committed bank after a reboot. */
 etch_store_project_select(2);etch_store_project_name("NEW NAME");make_note(92);
 writes=0;fail_at=15;
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)!=ETCH_STORE_OK);
 fail_at=-1;check(2,62);
 assert(memcmp(flash+ETCH_STORE_BASE,non_owned_before,sizeof(non_owned_before))==0);
 puts("PASS V0.86: explicit claim only, preserved Felucca project/sample1/presets/OTA");
 puts("PASS V0.86: four CRC projects, last-slot reload, interrupted rewrite rollback");
 return 0;
}
