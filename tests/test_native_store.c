#include "../firmware/fm1-native/wave_store.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static uint8_t flash[2*ETCH_STORE_BANK_BYTES],baseline[2*ETCH_STORE_BANK_BYTES];
static struct etch_wave_library one,two,loaded;
static int writes,fail_at=-1,partial,io_fail;
static int read_mem(uint32_t off,void *dst,uint32_t n){assert(off>=ETCH_STORE_BASE&&off+n<=ETCH_STORE_END);if(io_fail)return -1;memcpy(dst,flash+off-ETCH_STORE_BASE,n);return 0;}
static int erase_mem(uint32_t off){assert(!(off&4095));assert(off>=ETCH_STORE_BASE&&off+4096<=ETCH_STORE_END);if(writes++==fail_at){memset(flash+off-ETCH_STORE_BASE,255,partial?2048:0);return -1;}memset(flash+off-ETCH_STORE_BASE,255,4096);return 0;}
static int program_mem(uint32_t off,const void *src,uint32_t n){assert(off>=ETCH_STORE_BASE&&off+n<=ETCH_STORE_END);assert(n<=256&&((off&255)+n)<=256);int fail=writes++==fail_at;uint32_t count=fail?(partial?(n/2):0):n;const uint8_t *p=src;for(uint32_t i=0;i<count;i++){assert((flash[off-ETCH_STORE_BASE+i]&p[i])==p[i]);flash[off-ETCH_STORE_BASE+i]&=p[i];}return fail?-1:0;}
static const struct etch_store_io io={read_mem,erase_mem,program_mem};
int main(void){
 memset(flash,255,sizeof(flash));assert(etch_store_load(&io,&loaded)==ETCH_STORE_EMPTY);
 one.slot[0][4]=12000;one.active[0][4]=12000;one.valid[0]=1;
 assert(etch_store_save(&io,&one)==0);assert(etch_store_load(&io,&loaded)==0);assert(!memcmp(&one,&loaded,sizeof(one)));
 memcpy(baseline,flash,sizeof(flash));two=one;two.slot[63][8]=-13000;two.valid[1]=0x80000000u;two.selection[2]=63;
 writes=0;assert(etch_store_save(&io,&two)==0);int operations=writes;
 for(int cut=0;cut<operations;cut++)for(int p=0;p<2;p++){
  memcpy(flash,baseline,sizeof(flash));writes=0;fail_at=cut;partial=p;
  assert(etch_store_save(&io,&two)!=0);fail_at=-1;
  assert(etch_store_load(&io,&loaded)==0);assert(!memcmp(&one,&loaded,sizeof(one)));
  /* A retry after a torn target must still succeed without erasing old bank. */
  assert(etch_store_save(&io,&two)==0);assert(etch_store_load(&io,&loaded)==0);assert(!memcmp(&two,&loaded,sizeof(two)));
 }
 memcpy(flash,baseline,sizeof(flash));writes=0;assert(etch_store_save(&io,&two)==0);flash[ETCH_STORE_BANK_BYTES+256+10]^=1;
 assert(etch_store_load(&io,&loaded)==0);assert(!memcmp(&one,&loaded,sizeof(one)));
 /* Sequence wrap must treat generation 0 as newer than 0xffffffff. */
 memcpy(flash,baseline,sizeof(flash));uint32_t wrap=0xffffffffu;memcpy(flash+8,&wrap,4);uint32_t crc=etch_store_crc(flash,28);memcpy(flash+28,&crc,4);
 assert(etch_store_save(&io,&two)==0);assert(etch_store_load(&io,&loaded)==0);assert(!memcmp(&two,&loaded,sizeof(two)));
 /* First-ever interrupted claim has no previous committed copy and locks. */
 memset(flash,255,sizeof(flash));writes=0;fail_at=6;partial=1;assert(etch_store_save(&io,&one)!=0);fail_at=-1;
 assert(etch_store_load(&io,&loaded)==ETCH_STORE_FOREIGN);writes=0;assert(etch_store_save(&io,&one)==ETCH_STORE_FOREIGN);assert(writes==0);
 memset(flash,255,sizeof(flash));flash[900]=0x22;writes=0;assert(etch_store_save(&io,&one)==ETCH_STORE_FOREIGN);assert(writes==0);
 io_fail=1;assert(etch_store_save(&io,&one)==ETCH_STORE_IO);assert(writes==0);io_fail=0;
 writes=0;assert(etch_store_initialize(&io,&one,0)==ETCH_STORE_FOREIGN);assert(writes==0);
 assert(etch_store_initialize(&io,&one,ETCH_STORE_ERASE_CONFIRM)==0);assert(etch_store_load(&io,&loaded)==0);assert(!memcmp(&one,&loaded,sizeof(one)));
 writes=0;assert(etch_store_initialize(&io,&two,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_FOREIGN);assert(writes==0);
 /* Compatibility: V0.32 v1 records must remain readable and migrate on SAVE. */
 memset(flash,255,sizeof(flash));memset(&one,0,sizeof(one));
 one.valid[0]=1;one.slot[0][8]=19000;one.selection[0]=0;
 struct {uint32_t magic,version,generation,bytes,crc,bank,reserved,hcrc;} old={0};
 old.magic=0x38575745u;old.version=1;old.generation=42;
 old.bytes=ETCH_STORE_V1_BYTES;old.bank=0;old.crc=etch_store_crc(&one,old.bytes);
 old.hcrc=etch_store_crc(&old,28);
 memcpy(flash,&old,sizeof(old));memcpy(flash+256,&one,old.bytes);
 assert(etch_store_load(&io,&loaded)==ETCH_STORE_OK);
 assert(loaded.slot[0][8]==19000&&!loaded.wave_names[0][0]&&!loaded.patches[0].valid);
 loaded.wave_names[0][0]='G';loaded.wave_names[0][1]='L';loaded.wave_names[0][2]='A';loaded.wave_names[0][3]='S';loaded.wave_names[0][4]='S';
 loaded.patches[0].valid=1;loaded.patches[0].name[0]='B';loaded.patches[0].morph=24321;
 loaded.sequence[0]=60;loaded.sequence[1]=255;
 assert(etch_store_save(&io,&loaded)==ETCH_STORE_OK);
 uint32_t newver;memcpy(&newver,flash+ETCH_STORE_BANK_BYTES+4,4);assert(newver==2);
 assert(etch_store_load(&io,&two)==ETCH_STORE_OK);
 assert(two.patches[0].valid&&two.patches[0].morph==24321&&two.wave_names[0][0]=='G');
 assert(two.sequence[0]==60&&two.sequence[1]==255);
 assert(!etch_store_patch_waves_ready(&two,two.active));
 for(uint32_t w=0;w<3;w++)two.selection[w]=0;
 for(uint32_t w=0;w<3;w++)for(uint32_t k=0;k<128;k++)two.active[w][k]=two.slot[0][k];
 assert(etch_store_patch_waves_ready(&two,two.active));
 two.active[1][9]++;assert(!etch_store_patch_waves_ready(&two,two.active));
 flash[ETCH_STORE_BANK_BYTES+256+2]^=1;
 assert(etch_store_load(&io,&two)==ETCH_STORE_OK);
 assert(two.slot[0][8]==19000&&!two.patches[0].valid); /* fallback v1 */
 puts("Native store: 64 slots, every write cut, CRC fallback, v1->v2 migration, 32 patch/name/sequence records passed");
}
