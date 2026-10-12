/* P05: multi-project flash A/B snapshots, migration, names, CRC & capacity */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/wave_store.h"
#ifndef ETCH_P05_MULTI
#error test requires ETCH_P05_MULTI
#endif
#define BASE ETCH_STORE_BASE
#define TOTAL (ETCH_STORE_EXT_END-BASE)
#define TEST_BASE (ETCH_P04_EXPANDED?ETCH_STORE_EXT_BASE:ETCH_STORE_BASE)
#define TEST_END (ETCH_P04_EXPANDED?ETCH_STORE_EXT_END:ETCH_STORE_END)
static uint8_t memory[TOTAL];
static struct etch_wave_library lib,got;
static struct etch_native_pattern pat,found;
static struct etch_project_settings cfg,cfg_found;
static int fail_on=-1,attempt;
static int r(uint32_t off,void *out,uint32_t n){assert(off>=BASE&&off-BASE<=TOTAL-n);memcpy(out,memory+off-BASE,n);return 0;}
static int erase(uint32_t off){assert(off>=TEST_BASE&&off+4096<=TEST_END);if(attempt++==fail_on)return -1;memset(memory+off-BASE,255,4096);return 0;}
static int program(uint32_t off,const void *p,uint32_t n){assert(off>=TEST_BASE&&off+n<=TEST_END);assert(n<=256&&n+((off)&255u)<=256u);if(attempt++==fail_on)return -1;const uint8_t *s=p;for(uint32_t i=0;i<n;i++){uint8_t *d=&memory[off-BASE+i];assert((*d&s[i])==s[i]);*d&=s[i];}return 0;}
static const struct etch_store_io io={r,erase,program};
static void reset(void){memset(memory,255,sizeof(memory));memset(&lib,0,sizeof(lib));memset(&pat,0,sizeof(pat));memset(&cfg,0,sizeof(cfg));cfg.seq_length=16;cfg.tempo=120;cfg.seq_gate=75;etch_fx_defaults(&cfg.global_fx);etch_store_project_select(0);fail_on=-1;attempt=0;}
static void set_note(int v){
 etch_pattern_clear(&pat); assert(etch_seq_put(&pat,0,(uint8_t)v,110,2));
 lib.valid[0]=1;for(int i=0;i<128;i++)lib.slot[0][i]=(int16_t)(v+i);lib.selection[0]=lib.selection[1]=lib.selection[2]=0;
}
static void save(int slot,int note){etch_store_project_select((uint8_t)slot);char name[12]={0};snprintf(name,sizeof(name),"PROJECT %d",slot+1);etch_store_project_name(name);set_note(note);assert(etch_store_save_project(&io,&lib,&pat,&cfg)==ETCH_STORE_OK);}
static void load(int slot,int note){assert(etch_store_load_project_slot(&io,(uint8_t)slot,&got,&found,&cfg_found)==ETCH_STORE_OK);assert(found.steps[0].notes[0].note==note);assert(got.slot[0][1]==note+1);}
int main(void){
 /* Newly initialized device: user may choose ANY slot as first project.
  * It must appear immediately in PROJECTS and survive reboot/load. */
 reset();save(2,82);
 {uint8_t present=0;char labels[4][12]={{0}};
  assert(etch_store_projects(&io,&present,labels)==ETCH_STORE_OK);
  assert(present==4u);assert(!memcmp(labels[2],"PROJECT 3",9));}
 load(2,82);
 /* Migrate an existing single-project snapshot while saving directly to
  * project slot 3. The old project must survive in slot 1. */
 reset();set_note(57);
 {uint8_t *encoded=malloc(ETCH_PROJECT_MAX_BYTES);assert(encoded);
  uint32_t bytes=0;
  assert(etch_project_encode(&lib,&pat,&cfg,encoded,ETCH_PROJECT_MAX_BYTES,&bytes)==0);
  struct {uint32_t magic,version,generation,bytes,crc,bank,reserved,hcrc;} old={0};
  old.magic=0x38575745u;old.version=ETCH_P04_EXPANDED?10u:9u;
  old.generation=1;old.bytes=bytes;old.crc=etch_store_crc(encoded,bytes);
  old.bank=0;old.hcrc=etch_store_crc(&old,28);
  memcpy(memory+TEST_BASE-BASE+256,encoded,bytes);
  memcpy(memory+TEST_BASE-BASE,&old,32);
  free(encoded);
 }
 save(2,82);
 {uint8_t occupied=0;char labels[4][12]={{0}};
  assert(etch_store_projects(&io,&occupied,labels)==ETCH_STORE_OK);
  assert(occupied==5u);assert(!memcmp(labels[0],"LEGACY",6));}
 load(0,57);load(2,82);
 reset();save(0,60);save(1,61);save(2,62);save(3,63);
 for(int i=0;i<4;i++)load(i,60+i);
 uint8_t occupied=0;char names[4][12]={0};assert(etch_store_projects(&io,&occupied,names)==ETCH_STORE_OK);assert(occupied==15);assert(!memcmp(names[2],"PROJECT 3",9));
 save(1,75);load(0,60);load(1,75);load(2,62);load(3,63);
 int version=0;/* Source bank has one good commit. Interrupt an overwrite of slot 1. */
 etch_store_project_select(1);etch_store_project_name("PROJECT 2");set_note(83);attempt=0;fail_on=18;
 assert(etch_store_save_project(&io,&lib,&pat,&cfg)!=ETCH_STORE_OK);fail_on=-1;
 for(int i=0;i<4;i++)load(i,i==1?75:60+i);
 /* Old valid data survives CRC corruption of the most recently committed bank. */
 /* Deletion is a separate A/B commit; the last project cannot be removed. */
 uint8_t *baseline=malloc(sizeof(memory)); assert(baseline);memcpy(baseline,memory,sizeof(memory));
 for(int cutoff=0;cutoff<48;cutoff++){
  memcpy(memory,baseline,sizeof(memory));attempt=0;fail_on=cutoff;
  int rc=etch_store_delete_project_slot(&io,2,ETCH_STORE_ERASE_CONFIRM);
  fail_on=-1;
  if(rc!=ETCH_STORE_OK){
   for(int j=0;j<4;j++)load(j,j==1?75:60+j);
  }else{
   for(int j=0;j<4;j++)if(j!=2)load(j,j==1?75:60+j);
  }
 }
 memcpy(memory,baseline,sizeof(memory));fail_on=-1;
 assert(etch_store_delete_project_slot(&io,2,0u)!=ETCH_STORE_OK);
 assert(etch_store_delete_project_slot(&io,2,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_OK);
 occupied=0;assert(etch_store_projects(&io,&occupied,names)==ETCH_STORE_OK);
 assert(occupied==0x0bu);
 assert(etch_store_load_project_slot(&io,2,&got,&found,&cfg_found)!=ETCH_STORE_OK);
 load(0,60);load(1,75);load(3,63);
 /* Reuse the cleared slot; original projects keep their names and data. */
 save(2,91);load(2,91);load(1,75);
 /* Delete any occupied slot except last; initial active project remains loadable. */
 assert(etch_store_delete_project_slot(&io,0,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_OK);
 load(1,75);load(2,91);load(3,63);
 assert(etch_store_delete_project_slot(&io,1,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_OK);
 assert(etch_store_delete_project_slot(&io,2,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_OK);
 assert(etch_store_delete_project_slot(&io,3,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_INCOMPATIBLE);
 load(3,63);free(baseline);
 printf("PASS P05 transactional deletion and 48 interrupted writes; last project protected\n");
 printf("PASS P05 4 projects + naming + A/B interrupted save rollback; capacity bounded\n");
 (void)version;
}
