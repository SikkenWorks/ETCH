/* ETCH V0.50 P04 expanded flash tests.  Must compile -DETCH_P04_EXPANDED=1.
 * NOR mock refuses EVERY write outside the new extended pair. */
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/wave_store.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if !ETCH_P04_EXPANDED
#error Must use -DETCH_P04_EXPANDED=1
#endif
#define MOCK_BASE ETCH_STORE_BASE
#define MOCK_SIZE (ETCH_STORE_EXT_END-MOCK_BASE)
static uint8_t mock[MOCK_SIZE], snapshot[MOCK_SIZE];
static struct etch_wave_library lib,got,legacy;
static struct etch_native_pattern pat,other;
static struct etch_project_settings cfg,loaded;
static unsigned ops,erases,programs;static int fail_at=-1,half_fail;
static uint32_t rnd=0x12345678u;
static uint32_t random32(void){rnd^=rnd<<13;rnd^=rnd>>17;rnd^=rnd<<5;return rnd;}
static int rd(uint32_t off,void *p,uint32_t n){
 assert(off>=MOCK_BASE && n<=MOCK_SIZE && off-MOCK_BASE<=MOCK_SIZE-n);
 memcpy(p,mock+off-MOCK_BASE,n);return 0;
}
static int er(uint32_t off){
 assert(!(off&4095));assert(off>=ETCH_STORE_EXT_BASE&&off+4096<=ETCH_STORE_EXT_END);
 ++erases;int fail=(int)ops++==fail_at;
 if(fail){if(half_fail)memset(mock+off-MOCK_BASE,255,2048);return -1;}
 memset(mock+off-MOCK_BASE,255,4096);return 0;
}
static int wr(uint32_t off,const void *p,uint32_t n){
 assert(off>=ETCH_STORE_EXT_BASE&&off+n<=ETCH_STORE_EXT_END);
 assert(n<=256&&((off&255u)+n)<=256);
 ++programs;int fail=(int)ops++==fail_at;
 uint32_t nn=fail?(half_fail?n/2:0):n;
 const uint8_t *b=p;
 for(uint32_t i=0;i<nn;i++){
  uint8_t *dst=&mock[off-MOCK_BASE+i];assert((*dst & b[i])==b[i]);*dst&=b[i];
 }
 return fail?-1:0;
}
static const struct etch_store_io io={rd,er,wr};
static void reset(void){memset(mock,255,sizeof(mock));memset(&lib,0,sizeof(lib));memset(&cfg,0,sizeof(cfg));memset(&pat,0,sizeof(pat));ops=erases=programs=0;fail_at=-1;half_fail=0;cfg.seq_length=256;cfg.seq_gate=75;cfg.tempo=120;etch_fx_defaults(&cfg.global_fx);}
static void dense(void){
 lib.valid[0]=lib.valid[1]=0xffffffffu;
 for(unsigned i=0;i<64;i++)for(unsigned j=0;j<128;j++)lib.slot[i][j]=(int16_t)(random32()>>16);
 for(unsigned st=0;st<256;st++){
  for(unsigned n=0;n<8;n++)assert(etch_seq_put(&pat,(uint16_t)st,(uint8_t)(40+n),100,2));
  pat.steps[st].lock_mask=63;
  for(unsigned k=0;k<6;k++)pat.steps[st].locks[k]=(uint16_t)(1500+500*k);
 }
}
static void check(void){
 assert(etch_store_load_project(&io,&got,&other,&loaded)==ETCH_STORE_OK);
 assert(memcmp(got.slot,lib.slot,sizeof(lib.slot))==0);
 assert(memcmp(&pat,&other,sizeof(pat))==0);
 assert(loaded.seq_length==cfg.seq_length);
}
int main(void){
 reset();dense();uint32_t needed=0;
 assert(ETCH_P04_CURRENT_WORST_BYTES <= ETCH_PROJECT_MAX_BYTES);
 assert(etch_project_measure(&lib,&pat,&cfg,&needed)==0);
 assert(needed>ETCH_STORE_BANK_BYTES-256 && needed<=ETCH_PROJECT_MAX_BYTES);
 /* Entire extended flash pair blank; first claim does not erase legacy. */
 uint8_t old[2*ETCH_STORE_BANK_BYTES];memset(old,0xAA,sizeof(old));
 memcpy(mock,old,sizeof(old));
 assert(etch_store_save_project(&io,&lib,&pat,&cfg)==ETCH_STORE_OK);
 assert(!memcmp(mock,old,sizeof(old)));
 check();
 assert(programs>0&&erases==ETCH_STORE_EXT_BANK_BYTES/4096u);
 assert(mock[ETCH_STORE_EXT_BASE-MOCK_BASE]=='E');
 uint32_t ver;memcpy(&ver,mock+ETCH_STORE_EXT_BASE-MOCK_BASE+4,4);assert(ver==10);
 /* 2nd save commits to B, old A remains active after interrupted writes. */
 memcpy(snapshot,mock,sizeof(mock));
 lib.slot[0][3]^=0x1234;
 ops=0;fail_at=-1;assert(etch_store_save_project(&io,&lib,&pat,&cfg)==ETCH_STORE_OK);
 unsigned writes_for_save=ops;
 check();
 /* corruption in B falls back to A; restore previous project for testing */
 mock[ETCH_STORE_EXT_BASE-MOCK_BASE+ETCH_STORE_EXT_BANK_BYTES+256+42]^=0x80;
 assert(etch_store_load_project(&io,&got,&other,&loaded)==ETCH_STORE_OK);
 assert(got.slot[0][3]!=lib.slot[0][3]);
 /* Validate every 10th interrupted write across the expanded B bank. */
 uint32_t checks=0;
 for(unsigned i=0;i<writes_for_save;i+=11){
  for(unsigned part=0;part<2;part++){
   memcpy(mock,snapshot,sizeof(mock));ops=0;fail_at=i;half_fail=part;
   assert(etch_store_save_project(&io,&lib,&pat,&cfg)!=ETCH_STORE_OK);
   fail_at=-1;
   assert(etch_store_load_project(&io,&got,&other,&loaded)==ETCH_STORE_OK);
   assert(got.slot[0][3]!=lib.slot[0][3]);
   assert(!memcmp(mock,old,sizeof(old)));
   ++checks;
  }
 }

 /* Genuine V9->V10 source migration: loading the legacy project does not
  * write, and a successful extended save leaves the source bytes untouched. */
 reset();
 {
  FILE *f=fopen("tests/fixtures/native-project-v9.bin","rb");assert(f);
  uint8_t payload[ETCH_STORE_BANK_BYTES-256];
  size_t n=fread(payload,1,sizeof(payload),f);assert(n>128);fclose(f);
  struct {uint32_t magic,version,generation,bytes,crc,bank,reserved,hcrc;} h={0};
  h.magic=0x38575745u;h.version=9;h.generation=77;h.bytes=(uint32_t)n;
  h.bank=0;h.crc=etch_store_crc(payload,(uint32_t)n);h.hcrc=etch_store_crc(&h,28);
  memcpy(mock,&h,sizeof(h));memcpy(mock+256,payload,n);
  uint8_t legacy_before[2*ETCH_STORE_BANK_BYTES];
  memcpy(legacy_before,mock,sizeof(legacy_before));
  assert(etch_store_load_project(&io,&got,&other,&loaded)==ETCH_STORE_OK);
  assert(loaded.source_version==9);
  ops=erases=programs=0;
  assert(etch_store_save_project(&io,&got,&other,&loaded)==ETCH_STORE_OK);
  assert(!memcmp(legacy_before,mock,sizeof(legacy_before)));
  assert(etch_store_load_project(&io,&got,&other,&loaded)==ETCH_STORE_OK);
  assert(loaded.source_version==12);
  assert(erases==ETCH_STORE_EXT_BANK_BYTES/4096u);
 }
 /* Foreign Felucca/user data must never be erased or reinterpreted as ours. */
 reset();mock[ETCH_STORE_EXT_BASE-MOCK_BASE+0x2345]=0x42;
 ops=erases=programs=0;
 assert(etch_store_save_project(&io,&lib,&pat,&cfg)==ETCH_STORE_FOREIGN);
 assert(etch_store_load_project(&io,&got,&other,&loaded)==ETCH_STORE_FOREIGN);
 assert(erases==0 && programs==0);
 /* A Felucca FSMP header is outside the expanded range, but its payload
  * overlaps us. Detect it before claiming even apparently erased sectors. */
 reset();{uint32_t sample=0x504D5346u;memcpy(mock+(0xA0000u-MOCK_BASE),&sample,4);}
 ops=erases=programs=0;
 assert(etch_store_save_project(&io,&lib,&pat,&cfg)==ETCH_STORE_FOREIGN);
 assert(etch_store_load_project(&io,&got,&other,&loaded)==ETCH_STORE_FOREIGN);
 assert(!erases&&!programs);
 /* First-ever interrupted claim MUST NOT retry destructively. */
 reset();ops=0;fail_at=0;half_fail=1;
 assert(etch_store_save_project(&io,&lib,&pat,&cfg)!=ETCH_STORE_OK);
 fail_at=-1;
 /* An early, partial erase of blank flash still leaves blank, so retest a
    torn PROGRAM operation instead (a true nonblank foreign state). */
 ops=0;fail_at=ETCH_STORE_EXT_BANK_BYTES/4096u;half_fail=1;
 assert(etch_store_save_project(&io,&lib,&pat,&cfg)!=ETCH_STORE_OK);
 uint32_t before=programs;fail_at=-1;
 assert(etch_store_save_project(&io,&lib,&pat,&cfg)==ETCH_STORE_FOREIGN);
 assert(programs==before);
 printf("PASS P04 Felucca-informed expanded A/B: cap=%u bound=%zu required=%u, old untouched, V9 migrated, CRC fallback, %u power-cuts, foreign-data refusal\n",ETCH_PROJECT_MAX_BYTES,(size_t)ETCH_P04_CURRENT_WORST_BYTES,needed,checks);
}
