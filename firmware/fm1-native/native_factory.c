/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH initial writable starter content. Copy-on-first-commit: generated in
 * RAM; only the explicit STORE action may commit this to previously erased NOR.
 */
#include "native_factory.h"
#include "native_tables.h"
static int16_t sat(int32_t n){return (int16_t)(n<-29000?-29000:n>29000?29000:n);}
static void name12(char dst[12],const char *src){
 uint8_t ended=0;for(uint32_t i=0;i<12;i++){if(ended){dst[i]=0;continue;}dst[i]=src[i];if(!src[i])ended=1;}dst[11]=0;
}
void etch_factory_seed(struct etch_wave_library *l){
 static const char *const names[16]={
  "SINE","SAW","TRIANGLE","SQUARE","PULSE","ORGAN","BELL","HOLLOW",
  "GLASS","TEETH","ASH","WIRE","DUST","CUT","INK","SCRATCH"
 };
 uint8_t *b=(uint8_t*)l;for(uint32_t i=0;i<sizeof(*l);i++)b[i]=0;
 for(uint32_t w=0;w<16;w++){
  name12(l->wave_names[w],names[w]);l->valid[w>>5]|=(1u<<(w&31));
  for(uint32_t i=0;i<128;i++){
   int32_t a=etch_tables[0][0][i*4],b2=etch_tables[0][0][(i*8)&511u],c=etch_tables[0][0][(i*12)&511u];
   int32_t v=0;switch(w){
    case 0:v=a;break;case 1:v=(int32_t)i*453-29000;break;
    case 2:v=i<64?(int32_t)i*906-29000:29000-(int32_t)(i-64)*906;break;
    case 3:v=i<64?29000:-29000;break;case 4:v=i<24?29000:-29000;break;
    case 5:v=a+b2/3+c/5;break;case 6:v=a/2+b2/4+c/2;break;
    case 7:v=a-b2/2;break;case 8:v=a+c/2;break;case 9:v=a+b2/2-c/3;break;
    case 10:v=((int32_t)i*453-29000)/2+a/2;break;
    case 11:v=a>0?26000:(a<-12000?-27000:0);break;
    case 12:v=a/2+b2/2+c/3;break;case 13:v=(i<64?28000:-28000)+a/5;break;
    case 14:v=(i<32||i>96?25000:-25000)+c/6;break;
    default:v=a+b2/2+c/2;break;
   }
   l->slot[w][i]=sat(v);
  }
 }
 l->selection[0]=0;l->selection[1]=1;l->selection[2]=8;
 for(uint32_t w=0;w<3;w++)for(uint32_t i=0;i<128;i++)l->active[w][i]=l->slot[l->selection[w]][i];
 for(uint32_t p=0;p<ETCH_PATCH_SLOTS;p++){
  struct etch_native_patch *r=&l->patches[p];r->valid=1;
  r->name[0]='E';r->name[1]='T';r->name[2]='C';r->name[3]='H';r->name[4]=' ';
  r->name[5]=(char)('0'+p/10);r->name[6]=(char)('0'+p%10);r->name[7]=0;
  r->slot_a=p%16;r->slot_b=(p*5+3)%16;r->slot_c=(p*3+5)%16;
  r->morph=(uint16_t)((p%5)*6553);r->depth=(uint16_t)((p%4)*2048);
  r->mod_mode=0;r->mod_target=0;r->mod_ratio=(uint8_t)(p%6);r->filter_type=1;
  r->attack_ms=10;r->decay_ms=220;r->sustain=26000;r->release_ms=360;
  r->cutoff=7000;r->resonance=(uint16_t)((p%3)*2600);r->drive=(uint16_t)((p%3)*1024);
  r->chorus=(p%3==1?4096:0);r->delay=(p%6==3?4096:0);r->reverb=(p%4==0?4096:2048);
  r->seq_length=16;r->arp_mode=0;r->tempo=120;
 }
 /* V1.70: ready-to-play sound families instead of 32 anonymous ETCH nn
  * patches.  These templates are constructed in RAM on factory initialization
  * only; saved PJT31 projects and user presets are never migrated or reset. */
 static const char *const starter_names[16]={
  "WARM KEYS","GLASS PAD","DUST BASS","INK LEAD",
  "SOFT PULSE","AIR CHOIR","LOFI BELLS","HOLLOW PAD",
  "ORGAN KEYS","WIRE PLUCK","DEEP SPACE","SOFT EP",
  "CUT BASS","SLOW WAVE","SCRATCH FX","MELLOW LEAD"
 };
 static const uint8_t waves[16][3]={
  {0,5,8},{8,0,6},{1,7,0},{14,8,0},
  {4,0,7},{5,0,8},{6,8,0},{7,2,0},
  {5,0,6},{11,0,7},{8,7,0},{0,6,5},
  {13,0,6},{2,8,0},{15,7,0},{0,9,6}
 };
 static const uint16_t cutoff[16]={7800,5600,2300,8400,5800,6100,6600,4300,
                                     7500,6500,4900,6800,2100,5300,3900,7200};
 static const uint16_t attacks[16]={8,850,4,8,13,300,3,700,12,3,1300,15,3,900,22,7};
 static const uint16_t releases[16]={280,1850,180,220,360,1700,650,2200,
                                      350,240,2600,570,200,2100,750,340};
 for(uint8_t i=0u;i<16u;i++){
  struct etch_native_patch *r=&l->patches[i];
  name12(r->name,starter_names[i]);
  r->slot_a=waves[i][0];r->slot_b=waves[i][1];r->slot_c=waves[i][2];
  r->morph=(uint16_t)((i%4u)*8192u);
  r->depth=(uint16_t)((i==14u||i==6u)?4100u:1200u);
  r->attack_ms=attacks[i];r->decay_ms=230u;
  r->sustain=25500u;r->release_ms=releases[i];
  r->cutoff=cutoff[i];r->resonance=1400u;r->drive=0u;
  r->chorus=(i==1u||i==5u||i==7u||i==10u||i==13u)?3400u:0u;
  r->delay=(i==3u||i==6u||i==9u||i==14u)?2400u:0u;
  r->reverb=(i==1u||i==5u||i==7u||i==10u||i==13u)?6300u:1800u;
 }
 /* Add the new strings preset without disturbing any of the 16 existing
  * named patches or the 15 remaining legacy factory slots. */
 etch_factory_strings_patch(&l->patches[16]);
 for(uint32_t i=0;i<64;i++)l->sequence[i]=255;
}

/* Built-in preview can be applied to a *working sound* from any existing
 * PJT31 project. Nothing is written to the user's wave library or flash. */
void etch_factory_strings_patch(struct etch_native_patch *r){
 *r=(struct etch_native_patch){0};
 name12(r->name,"STRING ENS");r->valid=1u;
 r->slot_a=1u;r->slot_b=5u;r->slot_c=0u;
 r->morph=12500u;r->depth=0u;
 r->mod_mode=0u;r->mod_target=0u;r->mod_ratio=0u;r->filter_type=1u;
 r->attack_ms=320u;r->decay_ms=380u;r->sustain=28000u;r->release_ms=1450u;
 r->cutoff=6200u;r->resonance=350u;r->drive=0u;
 r->chorus=13000u;r->delay=0u;r->reverb=3400u;
 r->arp_mode=0u;r->seq_length=16u;r->tempo=120u;
}
void etch_factory_strings_wave(uint8_t wave,int16_t points[128]){
 for(uint32_t i=0;i<128u;i++){
  int32_t a=etch_tables[0][0][i*4u];
  int32_t b=etch_tables[0][0][(i*8u)&511u];
  int32_t c=etch_tables[0][0][(i*12u)&511u];
  int32_t n=wave==0u?(int32_t)i*453-29000:
            wave==1u?a+b/3+c/5:a;
  points[i]=sat(n);
 }
}
