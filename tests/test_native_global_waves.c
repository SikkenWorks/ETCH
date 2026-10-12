#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/wave_store.h"
#ifndef ETCH_P05_MULTI
#error Requires P05 catalog
#endif
#define BASE ETCH_STORE_BASE
#define SIZE (ETCH_STORE_EXT_END-ETCH_STORE_BASE)
#define TBASE (ETCH_P04_EXPANDED?ETCH_STORE_EXT_BASE:ETCH_STORE_BASE)
#define TEND (ETCH_P04_EXPANDED?ETCH_STORE_EXT_END:ETCH_STORE_END)
static uint8_t flash[SIZE];
static struct etch_wave_library lib,loaded;
static struct etch_native_pattern pattern,out_pattern;
static struct etch_project_settings cfg,out_cfg;
static uint8_t fail;
static int rd(uint32_t off,void *to,uint32_t n){assert(off>=BASE&&off+n<=BASE+SIZE);memcpy(to,flash+off-BASE,n);return 0;}
static int er(uint32_t off){assert(off>=TBASE&&off+4096<=TEND);if(fail)return -1;memset(flash+off-BASE,255,4096);return 0;}
static int wr(uint32_t off,const void *from,uint32_t n){assert(off>=TBASE&&off+n<=TEND);assert(n<=256);if(fail)return -1;
 const uint8_t *p=from;for(uint32_t i=0;i<n;i++){uint8_t *t=&flash[off-BASE+i];assert((*t&p[i])==p[i]);*t&=p[i];}return 0;}
static const struct etch_store_io io={rd,er,wr};
static void fill_wave(int v){lib.valid[0]=1;lib.selection[0]=lib.selection[1]=lib.selection[2]=0;
 for(int i=0;i<128;i++)lib.slot[0][i]=(int16_t)(v+i);memcpy(lib.active[0],lib.slot[0],sizeof(lib.slot[0]));
 lib.wave_names[0][0]='W';lib.wave_names[0][1]=(char)('0'+v/10);lib.wave_names[0][2]=(char)('0'+v%10);
 cfg.slot_fx[0].drive=(uint16_t)v;
}
static void save(int slot,int note){etch_store_project_select(slot);etch_store_project_name(slot?"PROJECT TWO":"PROJECT ONE");
 etch_pattern_clear(&pattern);assert(etch_seq_put(&pattern,0,note,110,1));
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)==ETCH_STORE_OK);}
static void load(int slot,int note,int wave){assert(etch_store_load_project_slot(&io,slot,&loaded,&out_pattern,&out_cfg)==ETCH_STORE_OK);
 assert(out_pattern.steps[0].notes[0].note==note);
 assert(loaded.slot[0][0]==wave);
 assert(loaded.slot[0][127]==wave+127);
 assert(out_cfg.slot_fx[0].drive==wave);
}
int main(void){memset(flash,255,sizeof(flash));memset(&lib,0,sizeof(lib));memset(&cfg,0,sizeof(cfg));
 cfg.seq_length=16;cfg.seq_gate=75;cfg.tempo=120;etch_fx_defaults(&cfg.global_fx);
 fill_wave(10);save(0,60);fill_wave(20);save(1,62);
 load(0,60,10);load(1,62,20); /* v11 compatibility: independent before explicit global wave SAVE. */
 etch_store_project_select(1);fill_wave(30);etch_store_global_wave_save();save(1,62);
 load(0,60,30);load(1,62,30); /* Shared master, distinct project pattern. */
 assert(etch_store_delete_project_slot(&io,1,ETCH_STORE_ERASE_CONFIRM)==ETCH_STORE_INCOMPATIBLE);
 assert(etch_store_load_project_slot(&io,0,&lib,&pattern,&cfg)==ETCH_STORE_OK);
 save(0,60);load(0,60,30);load(1,62,30); /* Master survives unrelated project save. */
 fail=1;etch_store_project_select(0);fill_wave(40);etch_store_global_wave_save();
 assert(etch_store_save_project(&io,&lib,&pattern,&cfg)!=ETCH_STORE_OK);etch_store_global_wave_cancel();fail=0;
 load(0,60,30);load(1,62,30); /* Interrupted save does not replace master. */
 puts("PASS global wave master follows wave SAVE across projects; V11 migration, V12 CRC, project independence and failed-write rollback");
 return 0;
}
