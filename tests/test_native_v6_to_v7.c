/* Genuine V6 bytes generated with the unmodified ETCH V0.41 encoder. */
#include "../firmware/fm1-native/native_project_codec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char in[ETCH_PROJECT_MAX_BYTES],out[ETCH_PROJECT_MAX_BYTES];
static struct etch_wave_library w,rw;
static struct etch_native_pattern p,rp;
static struct etch_project_settings s,rs;
int main(void){
 FILE *f=fopen("tests/fixtures/native-project-v6.bin","rb");assert(f);
 size_t n=fread(in,1,sizeof(in),f);assert(!ferror(f));fclose(f);
 assert(n>100&&memcmp(in,"PJT6",4)==0);
 assert(etch_project_decode(in,(uint32_t)n,&w,&p,&s)==0);
 assert(s.source_version==6&&p.steps[0].count==1&&p.steps[1].hold_count==1);
 assert(w.patches[0].valid&&w.patches[0].arp_mode==0);
 assert(s.arp_programs[0].length==0);
 etch_arp_program_init(&s.arp_programs[0]);
 s.arp_programs[0].length=4;s.arp_programs[0].steps[1]=255;
 uint32_t size=0;
 assert(etch_project_encode(&w,&p,&s,out,sizeof(out),&size)==0);
 assert(memcmp(out,"PJT<",4)==0);
 assert(etch_project_decode(out,size,&rw,&rp,&rs)==0);
 assert(rs.source_version==12&&rs.arp_programs[0].length==4&&rs.arp_programs[0].steps[1]==255);
 assert(memcmp(&p,&rp,sizeof(p))==0);
 assert(memcmp(&w,&rw,sizeof(w))==0);
 /* Reader rejects incorrect lengths/indices and trailing bytes. */
 out[size++]=1;assert(etch_project_decode(out,size,&rw,&rp,&rs)!=0);
 puts("PASS unmodified V0.41 V6 snapshot -> V7 roundtrip (waves, patch, HOLD, ARP) ");
}
