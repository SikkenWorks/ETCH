#include "../firmware/fm1-native/native_capacity.h"
#include "../firmware/fm1-native/native_project_codec.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static struct etch_wave_library waves;
static struct etch_native_pattern pat;
static struct etch_project_settings settings;
int main(void){
 struct etch_capacity_report a=etch_capacity_report(20224,20224);
 assert(a.fits&&a.overflow==0&&a.required==20224);
 a=etch_capacity_report(20225,20224);assert(!a.fits&&a.overflow==1);
 a=etch_capacity_report(30565,20224);assert(!a.fits&&a.overflow==10341);
 a=etch_capacity_report(UINT32_MAX,20224);assert(!a.fits&&a.overflow==UINT32_MAX-20224);
 settings.seq_length=256;settings.seq_gate=75;settings.tempo=120;
 for(unsigned step=0;step<256;step++){
  for(unsigned i=0;i<8;i++)assert(etch_seq_put(&pat,step,(uint8_t)(45+i),100,3));
  pat.steps[step].lock_mask=63;
  for(unsigned j=0;j<6;j++)pat.steps[step].locks[j]=1234+j;
 }
 waves.valid[0]=waves.valid[1]=UINT32_MAX;
 uint32_t rng=0x12345678u;
 for(unsigned i=0;i<64;i++)for(unsigned j=0;j<128;j++){
  rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
  waves.slot[i][j]=(int16_t)(rng>>16);
 }
 uint32_t bytes=0;
 assert(etch_project_measure(&waves,&pat,&settings,&bytes)==0);
 a=etch_capacity_report(bytes,ETCH_PROJECT_MAX_BYTES);
 assert(!a.fits&&a.overflow==bytes-ETCH_PROJECT_MAX_BYTES);
 puts("PASS P04: exact on-device capacity deficit and overflow arithmetic; max dense project refused");
 return 0;
}
