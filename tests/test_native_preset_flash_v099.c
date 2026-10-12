/* SPDX-License-Identifier: GPL-3.0-only
 * PJT13 fake-flash persistence and power-loss safety, no device writes.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/wave_store.h"
#if !ETCH_STORE_OWNED_ENABLED || !ETCH_P05_MULTI
#error "PJT13 must run against real owned-store project-bank layout"
#endif
static uint8_t flash[0x100000];
static struct etch_wave_library lib,got;
static struct etch_native_pattern pat,pat2;
static struct etch_project_settings settings,restore;
static int fail_after=-1,actions;
static int rd(uint32_t a,void *dst,uint32_t n){assert(a+n<=sizeof(flash));memcpy(dst,flash+a,n);return 0;}
static int er(uint32_t a){assert(!(a&4095)&&a>=ETCH_STORE_OWNED_BASE&&a+4096<=ETCH_STORE_OWNED_END);if(fail_after>=0&&actions++==fail_after)return -1;memset(flash+a,0xff,4096);return 0;}
static int wr(uint32_t a,const void *src,uint32_t n){assert(a>=ETCH_STORE_OWNED_BASE&&a+n<=ETCH_STORE_OWNED_END);if(fail_after>=0&&actions++==fail_after)return -1;for(unsigned i=0;i<n;i++){assert((flash[a+i]&((uint8_t*)src)[i])==((uint8_t*)src)[i]);flash[a+i]&=((uint8_t*)src)[i];}return 0;}
static const struct etch_store_io io={rd,er,wr};
int main(void){
 memset(flash,0xff,sizeof flash);memset(&lib,0,sizeof lib);memset(&settings,0,sizeof settings);
 etch_pattern_clear(&pat);settings.source_version=13;settings.seq_length=16;settings.seq_gate=75;settings.tempo=131;
 etch_fx_defaults(&settings.global_fx);
 lib.valid[0]=1;lib.patches[0].valid=1;lib.patches[7].valid=1;
 lib.selection[0]=lib.selection[1]=lib.selection[2]=0;
 settings.preset_valid=(1u<<0)|(1u<<7);
 settings.preset_fx[0].ring_mix=28000;settings.preset_fx[7].eq_high=21234;
 for(int p=0;p<8;p+=7)for(int w=0;w<3;w++)for(int i=0;i<128;i++)
  settings.preset_wave[p][w][i]=(int16_t)(p*1000+w*147-i*23);
 etch_store_project_select(0);etch_store_project_name("PRESET BANK");
 assert(etch_store_save_project(&io,&lib,&pat,&settings)==ETCH_STORE_OK);
 assert(etch_store_load_project(&io,&got,&pat2,&restore)==ETCH_STORE_OK);
 assert(restore.source_version==13&&restore.tempo==131&&restore.preset_valid==settings.preset_valid);
 assert(!memcmp(restore.preset_wave[7],settings.preset_wave[7],sizeof settings.preset_wave[7]));
 assert(restore.preset_fx[0].ring_mix==28000&&restore.preset_fx[7].eq_high==21234);
 /* Simulate a lost write during second save; first committed bank survives. */
 settings.tempo=172;settings.preset_fx[0].ring_mix=2;actions=0;fail_after=18;
 assert(etch_store_save_project(&io,&lib,&pat,&settings)!=ETCH_STORE_OK);
 fail_after=-1;
 assert(etch_store_load_project(&io,&got,&pat2,&restore)==ETCH_STORE_OK);
 assert(restore.tempo==131&&restore.preset_fx[0].ring_mix==28000);
 /* Subsequent healthy save works without erasing the old project catalogue. */
 assert(etch_store_save_project(&io,&lib,&pat,&settings)==ETCH_STORE_OK);
 assert(etch_store_load_project(&io,&got,&pat2,&restore)==ETCH_STORE_OK);
 assert(restore.tempo==172&&restore.preset_fx[0].ring_mix==2);
 puts("PASS V0.99 PJT13 fake-flash: independent full sound presets and power-loss rollback");
 return 0;
}
