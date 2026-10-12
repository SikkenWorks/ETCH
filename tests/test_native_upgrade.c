/* Build valid V4 and V3 snapshots from a V5-compatible prefix and verify
 * migrations preserve independent patterns, notes and wave data. */
#include "../firmware/fm1-native/native_project_codec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t flash[2*ETCH_STORE_BANK_BYTES],snapshot[2*ETCH_STORE_BANK_BYTES];
static struct etch_wave_library lib,loaded;
static struct etch_native_pattern pattern,out;
static struct etch_project_settings settings,readback;
struct header {uint32_t magic,version,generation,bytes,crc,bank,reserved,hcrc;};
static int rd(uint32_t off,void *p,uint32_t n){memcpy(p,flash+off-ETCH_STORE_BASE,n);return 0;}
static int er(uint32_t off){memset(flash+off-ETCH_STORE_BASE,255,4096);return 0;}
static int pr(uint32_t off,const void *p,uint32_t n){memcpy(flash+off-ETCH_STORE_BASE,p,n);return 0;}
static const struct etch_store_io io={rd,er,pr};
int main(void){
 memset(flash,255,sizeof(flash));
 FILE *fixture=fopen("tests/fixtures/native-project-v5.bin","rb");assert(fixture);
 size_t n=fread(flash+256,1,ETCH_PROJECT_MAX_BYTES,fixture);assert(!ferror(fixture));fclose(fixture);
 assert(n>175);
 struct header h={0x38575745u,5,1,(uint32_t)n,0,0,0,0};
 h.crc=etch_store_crc(flash+256,h.bytes);h.hcrc=etch_store_crc(&h,28);memcpy(flash,&h,sizeof(h));
 assert(etch_store_load_project(&io,&loaded,&out,&readback)==ETCH_STORE_OK);
 assert(readback.source_version==5&&out.steps[0].count==2);
 assert(out.steps[0].notes[0].note==60&&out.steps[0].notes[1].note==64);
 assert(out.steps[0].hold_count==0&&loaded.slot[0][0]==30001);
 memcpy(snapshot,flash,sizeof(flash));
 /* Authentic V5 -> synthetic V4: the FX settings are exactly 13 trailing bytes. */
 h.version=4;h.bytes-=13;
 flash[256]=0x50;flash[257]=0x4a;flash[258]=0x54;flash[259]=0x34;
 h.crc=etch_store_crc(flash+256,h.bytes);h.hcrc=etch_store_crc(&h,28);memcpy(flash,&h,sizeof(h));
 assert(etch_store_load_project(&io,&loaded,&out,&readback)==ETCH_STORE_OK);
 assert(readback.source_version==4&&out.steps[0].count==2);
 memcpy(flash,snapshot,sizeof(flash));memcpy(&h,flash,sizeof(h));
 /* V4 -> V3: archive with all slot lengths and per-patch rate/gate/swing = 175 bytes. */
 h.version=3;h.bytes-=175;
 flash[256]=0x50;flash[257]=0x4a;flash[258]=0x54;flash[259]=0x33;
 h.crc=etch_store_crc(flash+256,h.bytes);h.hcrc=etch_store_crc(&h,28);memcpy(flash,&h,sizeof(h));
 assert(etch_store_load_project(&io,&loaded,&out,&readback)==ETCH_STORE_OK);
 assert(readback.source_version==3&&out.steps[0].count==2);
 /* Upgrade to V7 without dropping either the old chord or new HOLD steps. */
 assert(etch_seq_hold_toggle(&out,1,60)==1);
 assert(etch_pattern_bank_capture(&readback.patterns,0,&out)==0);
 assert(etch_store_save_project(&io,&loaded,&out,&readback)==ETCH_STORE_OK);
 assert(etch_store_load_project(&io,&loaded,&out,&readback)==ETCH_STORE_OK);
 assert(readback.source_version==12&&out.steps[0].count==2&&out.steps[1].hold_count==1);
 assert(etch_pattern_bank_fetch(&readback.patterns,0,&pattern)==1);
 assert(pattern.steps[1].hold_count==1&&pattern.steps[1].holds[0]==60);
 puts("V3/V4/V5 -> V7 migration: genuine historical V5 sample, chord and authored HOLD preserved PASS");
}
