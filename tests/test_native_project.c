/* SPDX-License-Identifier: GPL-3.0-only
 * Exhaustive cut-at-every-flash-operation tests for V3 snapshots.
 */
#include "../firmware/fm1-native/native_project_codec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t flash[2*ETCH_STORE_BANK_BYTES],checkpoint[2*ETCH_STORE_BANK_BYTES];
static struct etch_wave_library lib1,lib2,loaded;
static struct etch_native_pattern pattern1,pattern2,patternloaded;
static struct etch_project_settings settings1,settings2,settingsloaded;
static int writes,fail=-1,partial;
static int readflash(uint32_t o,void *p,uint32_t n){assert(o>=ETCH_STORE_BASE&&o+n<=ETCH_STORE_END);memcpy(p,flash+o-ETCH_STORE_BASE,n);return 0;}
static int eraseflash(uint32_t o){assert(o>=ETCH_STORE_BASE&&o+4096<=ETCH_STORE_END&&!(o&4095));
 uint32_t n=(writes++==fail)?(partial?2048:0):4096;memset(flash+o-ETCH_STORE_BASE,255,n);return n==4096?0:-1;}
static int programflash(uint32_t o,const void *p,uint32_t n){assert(o>=ETCH_STORE_BASE&&o+n<=ETCH_STORE_END&&n<=256&&(o&255)+n<=256);
 int bad=(writes++==fail);uint32_t k=bad?(partial?n/2:0):n;
 for(uint32_t i=0;i<k;i++){uint8_t old=flash[o-ETCH_STORE_BASE+i],v=((const uint8_t*)p)[i];assert((old&v)==v);flash[o-ETCH_STORE_BASE+i]=old&v;}
 return bad?-1:0;
}
static const struct etch_store_io io={readflash,eraseflash,programflash};
static void check(const struct etch_wave_library *a,const struct etch_native_pattern *p,const struct etch_project_settings *s){
 assert(etch_store_load_project(&io,&loaded,&patternloaded,&settingsloaded)==ETCH_STORE_OK);
 assert(loaded.valid[0]==a->valid[0]&&loaded.valid[1]==a->valid[1]);
 assert(memcmp(loaded.active,a->active,sizeof(a->active))==0);
 assert(memcmp(loaded.wave_names,a->wave_names,sizeof(a->wave_names))==0);
 for(uint32_t k=0;k<64;k++)if((a->valid[k>>5]>>(k&31))&1u)assert(memcmp(loaded.slot[k],a->slot[k],256)==0);
 for(uint32_t k=0;k<32;k++)if(a->patches[k].valid)assert(memcmp(&loaded.patches[k],&a->patches[k],sizeof(a->patches[k]))==0);
 assert(memcmp(&patternloaded,p,sizeof(*p))==0);
 assert(memcmp(&settingsloaded,s,sizeof(*s))==0);
}
int main(void){
 memset(flash,255,sizeof(flash));assert(etch_store_load_project(&io,&loaded,&patternloaded,&settingsloaded)==ETCH_STORE_EMPTY);
 lib1.valid[0]=3;lib1.slot[0][0]=32000;lib1.slot[1][4]=-12000;
 memcpy(lib1.active[0],lib1.slot[0],256);strcpy(lib1.wave_names[0],"SWELL");
 lib1.patches[0].valid=1;lib1.patches[0].morph=19000;memcpy(lib1.patches[0].name,"WIDE\0",5);
 lib1.patches[0].flags=etch_arp_pack_flags(3,4,1); /* V0.41 packed ARP fields in old V6 record */
 etch_pattern_clear(&pattern1);assert(etch_seq_put(&pattern1,255,72,100,6));
 assert(etch_seq_put(&pattern1,255,76,110,8));pattern1.steps[255].lock_mask=0x21;
 pattern1.steps[255].locks[0]=32421;pattern1.steps[255].locks[5]=1024;
 settings1.source_version=12;etch_fx_defaults(&settings1.global_fx);settings1.seq_length=256;settings1.seq_gate=75;settings1.seq_swing=25;
 settings1.tempo=120;settings1.harmony.root=2;settings1.harmony.mode=1;
 settings1.harmony.keylock=2;settings1.wave_fx[0].chorus=3200;settings1.slot_fx[0].drive=1400;settings1.slot_fx[1].delay=2200;settings1.patch_fx_valid=1;settings1.patch_fx[0][0].bitcrush=17431;
 assert(etch_pattern_bank_capture(&settings1.patterns,0,&pattern1)==0);
 etch_arp_program_init(&settings1.arp_programs[0]);
 settings1.arp_programs[0].length=4;
 settings1.arp_programs[0].steps[0]=3;
 settings1.arp_programs[0].steps[1]=255;
 settings1.arp_programs[0].steps[2]=1;
 settings1.arp_programs[0].steps[3]=0;
 assert(etch_store_save_project(&io,&lib1,&pattern1,&settings1)==ETCH_STORE_OK);
 check(&lib1,&pattern1,&settings1);
 assert(loaded.patches[0].flags==etch_arp_pack_flags(3,4,1));
 memcpy(checkpoint,flash,sizeof(flash));lib2=lib1;pattern2=pattern1;settings2=settings1;
 lib2.valid[1]=0x80000000u;lib2.slot[63][22]=-32000;
 assert(etch_seq_put(&pattern2,0,48,80,3));pattern2.steps[0].lock_mask=0x3f;
 for(int i=0;i<6;i++)pattern2.steps[0].locks[i]=(uint16_t)(1000+i*200);
 assert(etch_pattern_bank_capture(&settings2.patterns,1,&pattern2)==0);
 settings2.pattern_rate[1]=3;settings2.pattern_gate[1]=92;settings2.pattern_swing[1]=17;
 etch_arp_program_init(&settings2.arp_programs[1]);
 settings2.arp_programs[1].steps[5]=255;
 settings2.global_fx.delay_time=32100;settings2.global_fx.routing=1;settings2.wave_fx[2].delay=32000;settings2.slot_fx[63].reverb=32767;settings2.seq_swing=40;
 writes=0;assert(etch_store_save_project(&io,&lib2,&pattern2,&settings2)==ETCH_STORE_OK);int operations=writes;
 check(&lib2,&pattern2,&settings2);
 for(int i=0;i<operations;i++)for(int p=0;p<2;p++){
  memcpy(flash,checkpoint,sizeof(flash));fail=i;partial=p;writes=0;
  assert(etch_store_save_project(&io,&lib2,&pattern2,&settings2)!=ETCH_STORE_OK);fail=-1;
  check(&lib1,&pattern1,&settings1);
  assert(etch_store_save_project(&io,&lib2,&pattern2,&settings2)==ETCH_STORE_OK);
  check(&lib2,&pattern2,&settings2);
 }
 /* Corrupt current CRC and confirm fallback to older committed project. */
 memcpy(flash,checkpoint,sizeof(flash));assert(etch_store_save_project(&io,&lib2,&pattern2,&settings2)==ETCH_STORE_OK);
 flash[ETCH_STORE_BANK_BYTES+256+10]^=1;check(&lib1,&pattern1,&settings1);
 /* A completely dense 256 x 8 note/6 lock pattern must fit with a small library. */
 memcpy(flash,checkpoint,sizeof(flash));lib2=lib1;settings2=settings1;
 etch_pattern_clear(&pattern2);
 for(int i=0;i<256;i++){
  for(int n=0;n<8;n++)assert(etch_seq_put(&pattern2,(uint16_t)i,(uint8_t)(40+n),100,8));
  pattern2.steps[i].lock_mask=63;
  for(int k=0;k<6;k++)pattern2.steps[i].locks[k]=(uint16_t)(1234+k);
 }
 assert(etch_store_save_project(&io,&lib2,&pattern2,&settings2)==ETCH_STORE_OK);
 check(&lib2,&pattern2,&settings2);
 /* Fully valid project larger than one bank is REJECTED without any writes. */
 memcpy(flash,checkpoint,sizeof(flash));
 lib2=lib1;lib2.valid[0]=lib2.valid[1]=0xffffffffu;
 uint32_t noise=0x12345678u;for(int i=0;i<64;i++)for(int j=0;j<128;j++){noise^=noise<<13;noise^=noise>>17;noise^=noise<<5;lib2.slot[i][j]=(int16_t)(noise>>16);}
 etch_pattern_clear(&pattern2);
 for(int i=0;i<256;i++){for(int n=0;n<8;n++)assert(etch_seq_put(&pattern2,(uint16_t)i,(uint8_t)(40+n),100,8));
 pattern2.steps[i].lock_mask=63;}
 writes=0;assert(etch_store_save_project(&io,&lib2,&pattern2,&settings2)==ETCH_STORE_FULL);assert(writes==0);
 check(&lib1,&pattern1,&settings1);
 /* Public legacy writer must not silently downgrade/truncate V3. */
 writes=0;assert(etch_store_save(&io,&lib1)==ETCH_STORE_INCOMPATIBLE);assert(writes==0);
 /* Legacy V2 loading into V3 API, then a V3 save. */
 memset(flash,255,sizeof(flash));assert(etch_store_save(&io,&lib1)==ETCH_STORE_OK);
 assert(etch_store_load_project(&io,&loaded,&patternloaded,&settingsloaded)==ETCH_STORE_OK);
 assert(settingsloaded.seq_length==64&&settingsloaded.tempo==120);
 assert(etch_store_save_project(&io,&lib1,&pattern1,&settings1)==ETCH_STORE_OK);
 check(&lib1,&pattern1,&settings1);
 printf("V10 project: 256-step poly, lock/FX/harmony roundtrip, %d fault cuts x2, CRC rollback, V2 upgrade, full-bank rejection PASS\n",operations);
 return 0;
}
