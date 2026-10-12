#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/wave_store.h"
#include "../firmware/fm1-native/native_project_codec.h"
#define BASE ETCH_STORE_BASE
#define SIZE (ETCH_STORE_EXT_END-ETCH_STORE_BASE)
static uint8_t flash[SIZE];
static struct etch_wave_library lib, out;
static struct etch_native_pattern pattern, out_pattern;
static struct etch_project_settings cfg, out_cfg;
static unsigned writes=0;
static int rd(uint32_t at,void *ptr,uint32_t n){assert(at>=BASE&&at+n<=BASE+SIZE);memcpy(ptr,flash+at-BASE,n);return 0;}
static int er(uint32_t at){assert(at>=BASE&&at+4096<=ETCH_STORE_END);memset(flash+at-BASE,255,4096);writes++;return 0;}
static int prog(uint32_t at,const void *ptr,uint32_t n){assert(at>=BASE&&at+n<=ETCH_STORE_END);assert(n<=256);const uint8_t *p=ptr;for(uint32_t k=0;k<n;k++){uint8_t *t=&flash[at-BASE+k];assert((*t&p[k])==p[k]);*t &= p[k];}writes++;return 0;}
static const struct etch_store_io io={rd,er,prog};
int main(void){
 memset(flash,255,sizeof(flash));flash[5]=0x12;flash[4097]=0x42;
 memset(&lib,0,sizeof(lib));memset(&cfg,0,sizeof(cfg));etch_pattern_clear(&pattern);
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;etch_fx_defaults(&cfg.global_fx);
 lib.selection[0]=lib.selection[1]=lib.selection[2]=0;
 lib.valid[0]=1;lib.wave_names[0][0]='T';lib.wave_names[0][1]='E';lib.wave_names[0][2]='S';lib.wave_names[0][3]='T';
 for(unsigned i=0;i<128;i++)lib.slot[0][i]=(int16_t)i;
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)==ETCH_STORE_FOREIGN);
 assert(writes==0);
 assert(etch_store_initialize_project(&io,&lib,&pattern,&cfg,0u)==ETCH_STORE_FOREIGN);
 assert(writes==0);
 etch_store_project_select(0);etch_store_project_name("NEW ETCH");
 int v=etch_store_initialize_project(&io,&lib,&pattern,&cfg,ETCH_STORE_ERASE_CONFIRM);
 assert(v==ETCH_STORE_OK);
 assert(writes>0);
 assert(etch_store_load_project_slot(&io,0,&out,&out_pattern,&out_cfg)==ETCH_STORE_OK);
 assert(!memcmp(out.wave_names[0],"TEST",4));
 unsigned prior=writes;
 assert(etch_store_initialize_project(&io,&lib,&pattern,&cfg,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_INCOMPATIBLE);
 assert(writes==prior);
 puts("PASS explicit foreign-data claim only with confirmation; successful project+wave read-back; subsequent valid ETCH data protected");
 return 0;
}
