/* SPDX-License-Identifier: GPL-3.0-only */
#include "native_project_codec.h"
#define V3MAGIC 0x33544a50u /* P J T 3, little endian */
#define V4MAGIC 0x34544a50u /* P J T 4, little endian */
#define V5MAGIC 0x35544a50u /* P J T 5, backward-readable */
#define V6MAGIC 0x36544a50u /* P J T 6, separate authored HOLD lane */
#define V7MAGIC 0x37544a50u /* P J T 7, per-patch custom ARP programs */
#define V8MAGIC 0x38544a50u /* P J T 8, linked per-trigger HOLD instances */
#define V9MAGIC 0x39544a50u /* P J T 9, lossless optional waveform delta-varint */
#define V10MAGIC 0x3a544a50u /* P J T : (10), optional lossless pattern archive LZ */
#define V11MAGIC 0x3b544a50u /* PJT11: additive project FX extension */
#define V12MAGIC 0x3c544a50u /* PJT12: one project-scoped voice-mode byte */
#define V13MAGIC 0x3d544a50u /* PJT13: independent preset FX and 3 exact waveforms */
#define V14MAGIC 0x3e544a50u /* PJT14: six global and preset audio FX */
#define V15MAGIC 0x3f544a50u /* PJT15: tape instability; additive eight bytes per FX */
#define V16MAGIC 0x40544a50u /* PJT16: reverse/vibrato/rotary/wah, appended 32B per FX */
#define V17MAGIC 0x41544a50u /* PJT17: pre-morph A/B inserts and Wave C FX mod */
#define V18MAGIC 0x42544a50u /* PJT18: four independent motion lanes */
#define V19MAGIC 0x43544a50u /* PJT19: Wave Evolution four-byte append */
#define V20MAGIC 0x44544a50u /* PJT20: expanded Wave C destinations; no new bytes */
#define V21MAGIC 0x45544a50u /* PJT21: 32 independent riff seeds */
#define V22MAGIC 0x46544a50u /* PJT22: 4 Organic Engine controls */
#define V23MAGIC 0x47544a50u /* PJT23: 4 Living Waves controls */
#define V24MAGIC 0x48544a50u /* PJT24: 4 matrix routes (12 bytes) */
#define V25MAGIC 0x49544a50u /* PJT25: per-patch RIFF 2.0 motif/density */
#define V26MAGIC 0x4a544a50u /* PJT26: per-preset DSP state */
#define V27MAGIC 0x4b544a50u /* PJT27: independent SPECTRAL K3/K4 values */
#define V28MAGIC 0x4c544a50u /* PJT28: per-preset expressive velocity curve */
#define V29MAGIC 0x4d544a50u /* PJT29: two key ranges and Group patch references */
#define V30MAGIC 0x4e544a50u /* PJT30: signed routes, aftertouch and keytrack. Same record size. */
#define V31MAGIC 0x4f544a50u /* PJT31 */
#define V32MAGIC 0x50544a50u /* PJT32: packed Wave AMP mode/depth per preset */
static const struct etch_project_settings *riff_owner;
static uint16_t *riff_data;
static const struct etch_project_settings *riff2_owner;
static struct etch_riff2_bank *riff2_data;
static struct {const struct etch_project_settings *p;struct etch_preset_dsp_bank *bank;} dsp_bindings[3];
void etch_project_bind_preset_dsp(struct etch_project_settings *p,struct etch_preset_dsp_bank *b){
 for(uint8_t i=0;i<3u;i++)if(dsp_bindings[i].p==p){dsp_bindings[i].bank=b;return;}
 for(uint8_t i=0;i<3u;i++)if(!dsp_bindings[i].p){dsp_bindings[i].p=p;dsp_bindings[i].bank=b;return;}
}
static struct etch_preset_dsp_bank *etch_dsp_bound(const struct etch_project_settings *p){
 for(uint8_t i=0;i<3u;i++)if(dsp_bindings[i].p==p)return dsp_bindings[i].bank;
 return 0;
}
void etch_project_copy_preset_dsp(struct etch_project_settings *to,const struct etch_project_settings *from){
 struct etch_preset_dsp_bank *src=etch_dsp_bound(from),*dst=etch_dsp_bound(to);
 if(src&&dst&&src!=dst)*dst=*src;
}
void etch_project_bind_riff2(struct etch_project_settings *project,struct etch_riff2_bank *bank){
 riff2_owner=project;riff2_data=bank;
}

void etch_project_bind_riffs(struct etch_project_settings *project,uint16_t *seeds){
 riff_owner=project;riff_data=seeds;
}
struct writer { uint8_t *p;uint32_t pos,cap;int error; };
struct reader { const uint8_t *p;uint32_t pos,cap;int error; };
static void zeromem(void *p,uint32_t n){uint8_t *b=p;for(uint32_t i=0;i<n;i++)b[i]=0;}
static void put(struct writer *w,const void *p,uint32_t n){
 if(w->error||n>w->cap-w->pos){w->error=1;return;}
 if(w->p){const uint8_t *b=p;for(uint32_t i=0;i<n;i++)w->p[w->pos+i]=b[i];}
 w->pos+=n;
}
static void get(struct reader *r,void *p,uint32_t n){
 if(r->error||n>r->cap-r->pos){r->error=1;return;}
 uint8_t *b=p;for(uint32_t i=0;i<n;i++)b[i]=r->p[r->pos+i];r->pos+=n;
}
static void put8(struct writer *w,uint8_t x){put(w,&x,1);}
static void put16(struct writer *w,uint16_t x){put8(w,(uint8_t)x);put8(w,(uint8_t)(x>>8));}
static void put32(struct writer *w,uint32_t x){put16(w,(uint16_t)x);put16(w,(uint16_t)(x>>16));}
static uint8_t get8(struct reader *r){uint8_t x=0;get(r,&x,1);return x;}
static uint16_t get16(struct reader *r){uint16_t x=get8(r);return x|(uint16_t)((uint16_t)get8(r)<<8);}
static uint32_t get32(struct reader *r){uint32_t x=get16(r);return x|((uint32_t)get16(r)<<16);}
static int validsettings(const struct etch_project_settings *s){
 return s->seq_length>=1&&s->seq_length<=ETCH_SEQ_STEPS&&s->tempo>=1&&
 s->seq_gate<=100&&s->seq_swing<=75&&s->clock_follow<=1&&s->mono_mode<=1&&
 s->harmony.root<12&&s->harmony.mode<9&&s->harmony.keylock<=2&&s->harmony.chord<=2&&
 s->global_fx.routing<=1&&s->global_fx.chorus_rate<=32767&&s->global_fx.chorus_depth<=32767&&
 s->global_fx.delay_time<=32767&&s->global_fx.delay_feedback<=32767&&
 s->global_fx.reverb_size<=32767&&s->global_fx.reverb_decay<=32767&&
 s->global_fx.ring_mix<=32767&&s->global_fx.ring_rate<=32767&&
 s->global_fx.ring_shape<=32767&&s->global_fx.ring_bias<=32767&&
 s->global_fx.eq_low<=32767&&s->global_fx.eq_mid<=32767&&
 s->global_fx.eq_high<=32767&&s->global_fx.eq_hpf<=32767;
}

/* PJT14 extension: 48 bytes of extra global FX per populated preset.
 * The original PJT13 preset FX prefix MUST remain exactly 30 bytes,
 * otherwise existing saved projects become undecodable. */
#define ETCH_FX_V13_SIZE 30u
_Static_assert(sizeof(struct etch_fx_settings)==128,"PJT17 FX extension size must stay tightly packed");
_Static_assert(__builtin_offsetof(struct etch_fx_settings,pre)==118,"PJT16 FX prefix must not move");
_Static_assert(__builtin_offsetof(struct etch_fx_settings,comp_threshold)==ETCH_FX_V13_SIZE,
 "PJT13 preset FX prefix ABI must remain byte-identical");
static void fx14_put(struct writer *w,const struct etch_fx_settings *c){
 put16(w,c->comp_threshold);
 put16(w,c->comp_ratio);
 put16(w,c->comp_attack);
 put16(w,c->comp_release);
 put16(w,c->trem_rate);
 put16(w,c->trem_depth);
 put16(w,c->trem_shape);
 put16(w,c->trem_mix);
 put16(w,c->phaser_rate);
 put16(w,c->phaser_depth);
 put16(w,c->phaser_feedback);
 put16(w,c->phaser_mix);
 put16(w,c->flanger_rate);
 put16(w,c->flanger_depth);
 put16(w,c->flanger_feedback);
 put16(w,c->flanger_mix);
 put16(w,c->decim_bits);
 put16(w,c->decim_rate);
 put16(w,c->decim_drive);
 put16(w,c->decim_mix);
 put16(w,c->folder_fold);
 put16(w,c->folder_symmetry);
 put16(w,c->folder_tone);
 put16(w,c->folder_mix);
}
static void fx14_get(struct reader *r,struct etch_fx_settings *c){
 c->comp_threshold=get16(r);
 c->comp_ratio=get16(r);
 c->comp_attack=get16(r);
 c->comp_release=get16(r);
 c->trem_rate=get16(r);
 c->trem_depth=get16(r);
 c->trem_shape=get16(r);
 c->trem_mix=get16(r);
 c->phaser_rate=get16(r);
 c->phaser_depth=get16(r);
 c->phaser_feedback=get16(r);
 c->phaser_mix=get16(r);
 c->flanger_rate=get16(r);
 c->flanger_depth=get16(r);
 c->flanger_feedback=get16(r);
 c->flanger_mix=get16(r);
 c->decim_bits=get16(r);
 c->decim_rate=get16(r);
 c->decim_drive=get16(r);
 c->decim_mix=get16(r);
 c->folder_fold=get16(r);
 c->folder_symmetry=get16(r);
 c->folder_tone=get16(r);
 c->folder_mix=get16(r);
}
static int fx15_valid(const struct etch_fx_settings *c){
 return c->tape_wow<=32767u&&c->tape_flutter<=32767u&&c->tape_age<=32767u&&c->tape_mix<=32767u;
}
static int fx14_valid(const struct etch_fx_settings *c){return !(c->comp_threshold>32767 || c->comp_ratio>32767 || c->comp_attack>32767 || c->comp_release>32767 || c->trem_rate>32767 || c->trem_depth>32767 || c->trem_shape>32767 || c->trem_mix>32767 || c->phaser_rate>32767 || c->phaser_depth>32767 || c->phaser_feedback>32767 || c->phaser_mix>32767 || c->flanger_rate>32767 || c->flanger_depth>32767 || c->flanger_feedback>32767 || c->flanger_mix>32767 || c->decim_bits>32767 || c->decim_rate>32767 || c->decim_drive>32767 || c->decim_mix>32767 || c->folder_fold>32767 || c->folder_symmetry>32767 || c->folder_tone>32767 || c->folder_mix>32767);}
static int fxnonzero(const struct etch_wave_fx_params *p){
 const uint16_t *v=(const uint16_t*)p;
 for(uint32_t i=0;i<8;i++)if(v[i])return 1;
 return 0;
}
static int bit(const struct etch_wave_library *l,uint32_t i){return (l->valid[i>>5]>>(i&31))&1u;}
/* Positive/negative predictor differences fit in 17 bits; the codec never
 * truncates or quantizes any signed 16-bit original waveform sample. */
static uint32_t zigzag(int32_t d){return d>=0?(uint32_t)(2*d):(uint32_t)(-2*d-1);}
static uint32_t wave_bytes(const int16_t *a){
 uint32_t n=2;
 for(uint32_t i=1;i<128;i++){
  uint32_t z=zigzag((int32_t)a[i]-a[i-1]);
  do{n++;z>>=7;}while(z);
 }
 return n;
}
static int wave_is_linear(const int16_t *a){
 int32_t d=(int32_t)a[1]-a[0];
 for(uint32_t i=2;i<128;i++)if((int32_t)a[i]-a[i-1]!=d)return 0;
 return d>=-65535&&d<=65535;
}
static void wave_put_compact(struct writer *w,const int16_t *a){
 put16(w,(uint16_t)a[0]);
 for(uint32_t i=1;i<128;i++){
  uint32_t z=zigzag((int32_t)a[i]-a[i-1]);
  do{uint8_t b=(uint8_t)(z&127u);z>>=7;put8(w,(uint8_t)(b|(z?128u:0u)));}while(z);
 }
}
static void wave_get_compact(struct reader *r,int16_t *a){
 a[0]=(int16_t)get16(r);
 for(uint32_t i=1;i<128;i++){
  uint32_t z=0;uint8_t done=0;
  for(uint32_t k=0;k<3;k++){
   uint8_t b=get8(r);
   if(r->error)return;
   if(k==2&&(b&120u)){r->error=1;return;} /* 17-bit bound */
   z|=(uint32_t)(b&127u)<<(k*7u);
   if(!(b&128u)){done=1;break;}
  }
  if(!done){r->error=1;return;}
  int32_t d=(z&1u)?-(int32_t)(z>>1)-1:(int32_t)(z>>1);
  int32_t result=(int32_t)a[i-1]+d;
  if(result<-32768||result>32767){r->error=1;return;}
  a[i]=(int16_t)result;
 }
}

/* PJT10 compact archive codec. 0..127: literal (tag+1) bytes.
 * 128..191: one byte repeated (tag&63)+3 times.
 * 192..255: backward copy (tag&63)+3 bytes, followed by distance 1..255.
 * No heap, bounded output, decoder verifies exact length and every reference.
 * All matches are byte-for-byte; worst-case input uses the raw archive instead. */
/* One previous matching 3-byte prefix per hash bucket. Unlike a quadratic
 * history scan this always does bounded work per byte (max 66 comparisons).
 * 1.5 KiB scratch, used only by non-realtime SAVE/preflight. */
static uint16_t lz_recent[768] __attribute__((section(".noinit")));
static uint16_t lz_hash(const uint8_t *data,uint32_t at){
 uint16_t key=(uint16_t)(((uint32_t)data[at]*331u+(uint32_t)data[at+1]*59u+data[at+2])&1023u);
 return key>=768u?(uint16_t)(key-768u):key;
}
static uint32_t lz_match(const uint8_t *data,uint32_t len,uint32_t at,uint32_t *distance){
 *distance=0;
 if(at+2>=len)return 0;
 uint16_t prev=lz_recent[lz_hash(data,at)];
 if(prev==0xffffu||prev>=at||at-prev>255u)return 0;
 if(data[at]!=data[prev]||data[at+1]!=data[prev+1]||data[at+2]!=data[prev+2])return 0;
 uint32_t k=3;
 while(k<66u&&at+k<len&&data[at+k]==data[prev+k])k++;
 *distance=at-prev;return k;
}
static void lz_remember(const uint8_t *data,uint32_t len,uint32_t start,uint32_t end){
 for(uint32_t i=start;i<end;i++)if(i+2<len)lz_recent[lz_hash(data,i)]=(uint16_t)i;
}
static uint32_t lz_run(const uint8_t *data,uint32_t len,uint32_t at){
 uint32_t k=1;while(k<66u&&at+k<len&&data[at+k]==data[at])k++;return k;
}
/* dst==NULL performs exactly the same token-selection pass for sizing. */
static uint32_t archive_lz_pack(const uint8_t *data,uint32_t len,uint8_t *dst,uint32_t cap){
 uint32_t at=0,out=0,lit_start=0,lit_len=0;
 for(uint32_t i=0;i<768;i++)lz_recent[i]=0xffffu;
 while(at<len){
  uint32_t start=at,dist=0,match=lz_match(data,len,at,&dist),run=lz_run(data,len,at);
  if(match>=3||run>=3||lit_len==128){
   if(lit_len){
    if(out>cap||lit_len+1>cap-out)return UINT32_MAX;
    if(dst){dst[out]=(uint8_t)(lit_len-1);for(uint32_t i=0;i<lit_len;i++)dst[out+1+i]=data[lit_start+i];}
    out+=lit_len+1;lit_len=0;
   }
   if(run>=3&&run>=match){
    if(out>cap||2>cap-out)return UINT32_MAX;
    if(dst){dst[out]=(uint8_t)(0x80u|(run-3));dst[out+1]=data[at];}
    out+=2;at+=run;
   }else if(match>=3){
    if(out>cap||2>cap-out)return UINT32_MAX;
    if(dst){dst[out]=(uint8_t)(0xc0u|(match-3));dst[out+1]=(uint8_t)dist;}
    out+=2;at+=match;
   }else{
    lit_start=at;lit_len=1;at++;
   }
  }else{if(!lit_len)lit_start=at;lit_len++;at++;}
  lz_remember(data,len,start,at);
 }
 if(lit_len){
  if(out>cap||lit_len+1>cap-out)return UINT32_MAX;
  if(dst){dst[out]=(uint8_t)(lit_len-1);for(uint32_t i=0;i<lit_len;i++)dst[out+1+i]=data[lit_start+i];}
  out+=lit_len+1;
 }
 return out;
}
static int archive_lz_unpack(const uint8_t *src,uint32_t size,uint8_t *dst,uint32_t expected){
 uint32_t at=0,out=0;
 while(at<size){
  uint8_t tag=src[at++];uint32_t n;
  if(tag<128){
   n=(uint32_t)tag+1;
   if(n>size-at||n>expected-out)return -1;
   for(uint32_t k=0;k<n;k++)dst[out+k]=src[at+k];
   at+=n;out+=n;
  }else if(tag<192){
   n=(tag&63u)+3u;
   if(at>=size||n>expected-out)return -1;
   uint8_t value=src[at++];for(uint32_t k=0;k<n;k++)dst[out++]=value;
  }else{
   n=(tag&63u)+3u;
   if(at>=size||n>expected-out)return -1;
   uint32_t dist=src[at++];if(!dist||dist>out)return -1;
   for(uint32_t k=0;k<n;k++){dst[out]=dst[out-dist];out++;}
  }
 }
 return out==expected?0:-1;
}
int etch_project_encode(const struct etch_wave_library *l,const struct etch_native_pattern *p,
 const struct etch_project_settings *s,uint8_t *dst,uint32_t cap,uint32_t *used){
 if(!l||!p||!s||!used||!validsettings(s))return -1;
 if(s->source_version>=19u&&!etch_evolution_valid(&s->evolution))return -1;
 if(s->source_version>=22u&&!etch_organic_valid(&s->organic))return -1;
 if(s->source_version>=23u&&!etch_living_valid(&s->living))return -1;
 if(s->source_version>=24u&&!etch_matrix_valid(&s->matrix))return -1;
 if(s->source_version>=24u&&s->source_version<30u&&!etch_matrix_valid_legacy(&s->matrix))return -1;
 for(uint32_t j=0;j<3;j++)if(l->selection[j]>=ETCH_WAVE_SLOTS)return -1;
 struct writer w={dst,0,cap,0};
 if(etch_pattern_bank_validate(&s->patterns))return -1;
 uint8_t write13=(uint8_t)(s->source_version>=13);
 uint8_t write14=(uint8_t)(s->source_version>=14);
 uint8_t write15=(uint8_t)(s->source_version>=15);
 uint8_t write16=(uint8_t)(s->source_version>=16);
 uint8_t write17=(uint8_t)(s->source_version>=17);
 uint8_t write18=(uint8_t)(s->source_version>=18);
 uint8_t write19=(uint8_t)(s->source_version>=19);
 uint8_t write20=(uint8_t)(s->source_version>=20);
 uint8_t write21=(uint8_t)(s->source_version>=21);
 uint8_t write22=(uint8_t)(s->source_version>=22);
 uint8_t write23=(uint8_t)(s->source_version>=23);
 uint8_t write24=(uint8_t)(s->source_version>=24);
 uint8_t write25=(uint8_t)(s->source_version>=25);
 uint8_t write26=(uint8_t)(s->source_version>=26);
 uint8_t write27=(uint8_t)(s->source_version>=27);
 uint8_t write28=(uint8_t)(s->source_version>=28);
 uint8_t write29=(uint8_t)(s->source_version>=29);
 uint8_t write30=(uint8_t)(s->source_version>=30);
 uint8_t write31=(uint8_t)(s->source_version>=31);
 uint8_t write32=(uint8_t)(s->source_version>=32);
 put32(&w,write32?V32MAGIC:write31?V31MAGIC:write30?V30MAGIC:write29?V29MAGIC:write28?V28MAGIC:write27?V27MAGIC:write26?V26MAGIC:write25?V25MAGIC:write24?V24MAGIC:write23?V23MAGIC:write22?V22MAGIC:write21?V21MAGIC:write20?V20MAGIC:write19?V19MAGIC:write18?V18MAGIC:write17?V17MAGIC:write16?V16MAGIC:write15?V15MAGIC:write14?V14MAGIC:write13?V13MAGIC:V12MAGIC);put8(&w,l->reserved);put(&w,l->selection,3);
 put32(&w,l->valid[0]);put32(&w,l->valid[1]);
 put(&w,l->active,sizeof(l->active));put(&w,l->wave_names,sizeof(l->wave_names));
 uint32_t compact[2]={0,0},linear[2]={0,0};
 for(uint32_t i=0;i<ETCH_WAVE_SLOTS;i++)if(bit(l,i)){
  if(wave_is_linear(l->slot[i])){compact[i>>5]|=1u<<(i&31);linear[i>>5]|=1u<<(i&31);}
  else if(wave_bytes(l->slot[i])<sizeof(l->slot[i]))compact[i>>5]|=1u<<(i&31);
 }
 put32(&w,compact[0]);put32(&w,compact[1]);put32(&w,linear[0]);put32(&w,linear[1]);
 for(uint32_t i=0;i<ETCH_WAVE_SLOTS;i++)if(bit(l,i)){
  if((linear[i>>5]>>(i&31))&1u){put16(&w,(uint16_t)l->slot[i][0]);put32(&w,(uint32_t)((int32_t)l->slot[i][1]-l->slot[i][0]));}
  else if((compact[i>>5]>>(i&31))&1u)wave_put_compact(&w,l->slot[i]);
  else put(&w,l->slot[i],sizeof(l->slot[i]));
 }
 uint32_t patchbits=0;
 for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if(l->patches[i].valid)patchbits|=1u<<i;
 put32(&w,patchbits);
 for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if((patchbits>>i)&1u)put(&w,&l->patches[i],sizeof(l->patches[i]));
 put(&w,l->sequence,sizeof(l->sequence));
 put16(&w,s->seq_length);put8(&w,s->seq_rate);put8(&w,s->seq_gate);
 put8(&w,s->seq_swing);put8(&w,s->tempo);put8(&w,s->clock_follow);
 put(&w,&s->harmony,sizeof(s->harmony));put(&w,s->wave_fx,sizeof(s->wave_fx));
 uint32_t fxbit[2]={0,0};
 for(uint32_t i=0;i<ETCH_WAVE_SLOTS;i++)if(fxnonzero(&s->slot_fx[i]))fxbit[i>>5]|=1u<<(i&31);
 put32(&w,fxbit[0]);put32(&w,fxbit[1]);
 for(uint32_t i=0;i<ETCH_WAVE_SLOTS;i++)if(fxbit[i>>5]&(1u<<(i&31)))put(&w,&s->slot_fx[i],sizeof(s->slot_fx[i]));
 if(s->patch_fx_valid&~patchbits)return -1;
 put32(&w,s->patch_fx_valid);
 for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if(s->patch_fx_valid&(1u<<i))put(&w,s->patch_fx[i],sizeof(s->patch_fx[i]));
 uint16_t occupied=0;
 for(uint32_t step=0;step<ETCH_SEQ_STEPS;step++){
  const struct etch_seq_step *st=&p->steps[step];
  if(st->count>ETCH_SEQ_NOTES||st->hold_count>ETCH_SEQ_NOTES||(st->lock_mask&~((1u<<ETCH_SEQ_LOCKS)-1u)))return -1;
  for(uint8_t i=0;i<st->hold_count;i++){
   if(st->holds[i]>127)return -1;
   if(st->hold_ago[i]){
    uint8_t origin=(uint8_t)(step-st->hold_ago[i]);const struct etch_seq_step *src=&p->steps[origin];uint8_t found=0;
    for(uint8_t j=0;j<src->count;j++)if(src->notes[j].note==st->holds[i])found=1;
    if(!found)return -1;
   }
   for(uint8_t j=0;j<i;j++)if(st->holds[i]==st->holds[j]&&(st->hold_ago[i]==st->hold_ago[j]||!st->hold_ago[i]||!st->hold_ago[j]))return -1;
  }
  for(uint32_t i=0;i<st->count;i++)if(st->notes[i].note>127||!st->notes[i].velocity||!st->notes[i].length)return -1;
  if(st->count||st->lock_mask||st->hold_count)occupied++;
 }
 put16(&w,occupied);
 for(uint32_t step=0;step<ETCH_SEQ_STEPS;step++){
  const struct etch_seq_step *st=&p->steps[step];
  if(!st->count&&!st->lock_mask&&!st->hold_count)continue;
  put8(&w,(uint8_t)step);put8(&w,st->count);put8(&w,st->lock_mask);put8(&w,st->hold_count);
  for(uint32_t i=0;i<st->count;i++)put(&w,&st->notes[i],sizeof(st->notes[i]));
  for(uint8_t i=0;i<st->hold_count;i++){put8(&w,st->holds[i]);put8(&w,st->hold_ago[i]);}
  for(uint32_t i=0;i<ETCH_SEQ_LOCKS;i++)if(st->lock_mask&(1u<<i))put16(&w,st->locks[i]);
 }
 put16(&w,s->patterns.used);
 for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)put16(&w,s->patterns.lengths[i]);
 /* Compression applies only to the archived per-patch data. We never silently
  * drop or alter a byte. The maximum archive is ETCH_PATTERN_ARCHIVE_BYTES by construction. */
 uint32_t packed_size=archive_lz_pack(s->patterns.data,s->patterns.used,0,UINT32_MAX);
 uint8_t compressed=packed_size<s->patterns.used;
 uint32_t archive_size=compressed?packed_size:s->patterns.used;
 put8(&w,compressed);put16(&w,(uint16_t)archive_size);
 if(compressed){
  if(w.error||archive_size>w.cap-w.pos){w.error=1;}
  else{
   if(w.p&&archive_lz_pack(s->patterns.data,s->patterns.used,w.p+w.pos,archive_size)!=archive_size)w.error=1;
   if(!w.error)w.pos+=archive_size;
  }
 }else put(&w,s->patterns.data,s->patterns.used);
 put(&w,s->pattern_rate,ETCH_PATCH_SLOTS);put(&w,s->pattern_gate,ETCH_PATCH_SLOTS);put(&w,s->pattern_swing,ETCH_PATCH_SLOTS);
 put16(&w,s->global_fx.chorus_rate);put16(&w,s->global_fx.chorus_depth);
 put16(&w,s->global_fx.delay_time);put16(&w,s->global_fx.delay_feedback);
 put16(&w,s->global_fx.reverb_size);put16(&w,s->global_fx.reverb_decay);
 put8(&w,s->global_fx.routing);
 /* V7 extension is sparse: legacy patches without an authored ARP cost zero
  * program steps. Store slot, length and each selected key index/rest. */
 uint8_t count=0;
 for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++){
  if(!etch_arp_program_valid(&s->arp_programs[i]))return -1;
  if(s->arp_programs[i].length)++count;
 }
 put8(&w,count);
 for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++)if(s->arp_programs[i].length){
  const struct etch_arp_program *a=&s->arp_programs[i];
  put8(&w,i);put8(&w,a->length);put(&w,a->steps,ETCH_ARP_PROGRAM_STEPS);
 }
 /* Appended after the complete legacy ARP section so older snapshot
  * formats are decoded without shifting any pre-existing fields. */
 put16(&w,s->global_fx.ring_mix);put16(&w,s->global_fx.ring_rate);
 put16(&w,s->global_fx.ring_shape);put16(&w,s->global_fx.ring_bias);
 put16(&w,s->global_fx.eq_low);put16(&w,s->global_fx.eq_mid);
 put16(&w,s->global_fx.eq_high);put16(&w,s->global_fx.eq_hpf);
 put8(&w,s->mono_mode);
 if(write13){
  /* Only snapshots actually saved by the user consume flash capacity. */
  if(s->preset_valid&~patchbits)return -1;
  put32(&w,s->preset_valid);
  for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if(s->preset_valid&(1u<<i)){
   const struct etch_fx_settings *fx=&s->preset_fx[i];
   if(fx->routing>1||fx->ring_mix>32767||fx->eq_low>32767||fx->eq_mid>32767||fx->eq_high>32767)return -1;
   put(&w,fx,ETCH_FX_V13_SIZE);
   for(uint32_t j=0;j<3;j++){
    const int16_t *wave=s->preset_wave[i][j];
    uint8_t method=wave_is_linear(wave)?2:(wave_bytes(wave)<sizeof(s->preset_wave[i][j])?1:0);
    put8(&w,method);
    if(method==2){put16(&w,(uint16_t)wave[0]);put32(&w,(uint32_t)((int32_t)wave[1]-wave[0]));}
    else if(method==1)wave_put_compact(&w,wave);
    else put(&w,wave,sizeof(s->preset_wave[i][j]));
   }
  }
 }
 if(write14){
  if(!fx14_valid(&s->global_fx))return -1;
  fx14_put(&w,&s->global_fx);
  for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if(s->preset_valid&(1u<<i)){
   if(!fx14_valid(&s->preset_fx[i]))return -1;
   fx14_put(&w,&s->preset_fx[i]);
  }
 }
 if(write15){
  if(!fx15_valid(&s->global_fx))return -1;
  for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if((s->preset_valid&(1u<<i))&&!fx15_valid(&s->preset_fx[i]))return -1;
  put16(&w,s->global_fx.tape_wow);put16(&w,s->global_fx.tape_flutter);
  put16(&w,s->global_fx.tape_age);put16(&w,s->global_fx.tape_mix);
  for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if(s->preset_valid&(1u<<i)){
   const struct etch_fx_settings *fx=&s->preset_fx[i];
   put16(&w,fx->tape_wow);put16(&w,fx->tape_flutter);
   put16(&w,fx->tape_age);put16(&w,fx->tape_mix);
  }
 }
 if(write16){
  const struct etch_fx_settings *fx=&s->global_fx;
  const uint16_t *params=&fx->reverse_time;
  for(uint32_t i=0;i<16;i++){if(params[i]>32767u)return -1;put16(&w,params[i]);}
  for(uint32_t slot=0;slot<ETCH_PATCH_SLOTS;slot++)if(s->preset_valid&(1u<<slot)){
   params=&s->preset_fx[slot].reverse_time;
   for(uint32_t i=0;i<16;i++){if(params[i]>32767u)return -1;put16(&w,params[i]);}
  }
 }
 if(write17){
  const struct etch_fx_settings *settings=&s->global_fx;
  for(uint32_t slot=0;slot<=ETCH_PATCH_SLOTS;slot++){
   if(slot){if(!(s->preset_valid&(1u<<(slot-1u))))continue;settings=&s->preset_fx[slot-1u];}
   for(uint32_t wave=0;wave<2u;wave++)for(uint32_t insert=0;insert<2u;insert++){
    if(etch_pre_type(settings,(uint8_t)wave,(uint8_t)insert)>5u)return -1;
    put16(&w,settings->pre[wave][insert]);
   }
   if(etch_c_fx_target(settings)>ETCH_C_FX_MAX_TARGET)return -1;
   if(!write20 && etch_c_fx_target(settings)>6u)return -1;
   put16(&w,write20?settings->c_fx:etch_c_fx_as_old(settings->c_fx));
  }
 }
 if(write18){
  if(!etch_motion_valid(&s->motion))return -1;
  put8(&w,s->motion.length);put8(&w,s->motion.mask);
  for(uint32_t k=0;k<4u;k++)for(uint32_t i=0;i<s->motion.length;i++)
   put16(&w,s->motion.lane[k][i]);
 }
 if(write19){put8(&w,s->evolution.depth);put8(&w,s->evolution.rate);
  put8(&w,s->evolution.mode);put8(&w,s->evolution.sync);}
 if(write21)for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++)put16(&w,(s==riff_owner&&riff_data)?riff_data[i]:0u);
 if(write22){put8(&w,s->organic.drift);put8(&w,s->organic.rate);
  put8(&w,s->organic.phase);put8(&w,s->organic.variance);}
 if(write23){put8(&w,s->living.depth);put8(&w,s->living.rate);
  put8(&w,s->living.color);put8(&w,s->living.vary);}
 if(write24)for(uint32_t i=0;i<ETCH_MATRIX_ROUTES;i++){
  put8(&w,s->matrix.route[i].source);put8(&w,s->matrix.route[i].target);put8(&w,s->matrix.route[i].amount);
 }
 if(write25)for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++){
  uint8_t density=(s==riff2_owner&&riff2_data)?riff2_data->riff_densities[i]:50u;
  uint8_t mutation=(s==riff2_owner&&riff2_data)?riff2_data->riff_mutations[i]:0u;
  uint16_t variant=(s==riff2_owner&&riff2_data)?riff2_data->riff_variants[i]:0u;
  if(density>100u||mutation>100u)return -1;
  put16(&w,variant);put8(&w,mutation);put8(&w,density);
 }
 if(write26){
  const struct etch_preset_dsp_bank *bank=etch_dsp_bound(s);
  if(!bank)return -1;
  for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++){
   const struct etch_preset_dsp *v=&bank->slot[i];
   if((!write30&&!etch_matrix_valid_legacy(&v->matrix))||
      !etch_organic_valid(&v->organic)||!etch_living_valid(&v->living)||
      !etch_matrix_valid(&v->matrix)||!etch_evolution_valid(&v->evolution)||
      (write32?(((v->spectral_wave&0xf0u)!=0u)||((v->spectral_wave&3u)>2u)):v->spectral_wave>2u)||v->spectral_part>32u||v->blend_a>=32u||
      v->blend_b>=32u||v->blend_pct>100u||v->blend_on>1u||
      (write27&&(v->spectral_gain>100u||v->spectral_tilt>100u))||(write28&&(write32?(v->velocity_curve&3u)>2u:v->velocity_curve>2u)))return -1;
   put(&w,v,write28?sizeof(*v):write27?32u:30u);
  }
 }
 if(write29){
  const struct etch_preset_dsp_bank *bank=etch_dsp_bound(s);
  if(!bank||!etch_key_groups_valid(&bank->groups))return -1;
  uint8_t packed[8];etch_key_groups_pack(&bank->groups,packed);
  put(&w,packed,8u);
 }
 if(write31){
  const struct etch_preset_dsp_bank *bank=etch_dsp_bound(s);
  if(!bank||!etch_perf_valid(&bank->performance))return -1;
  put(&w,&bank->performance,sizeof(bank->performance));
 }
 if(w.error)return -2;
 *used=w.pos;return 0;
}
int etch_project_decode(const uint8_t *src,uint32_t len,struct etch_wave_library *l,
 struct etch_native_pattern *p,struct etch_project_settings *s){
 if(!src||!l||!p||!s||len>ETCH_PROJECT_MAX_BYTES)return -1;
 struct reader r={src,0,len,0};
 uint32_t magic=get32(&r);if(magic!=V3MAGIC&&magic!=V4MAGIC&&magic!=V5MAGIC&&magic!=V6MAGIC&&magic!=V7MAGIC&&magic!=V8MAGIC&&magic!=V9MAGIC&&(magic!=V10MAGIC&&magic!=V11MAGIC&&magic!=V12MAGIC&&magic!=V13MAGIC&&(magic!=V14MAGIC&&magic!=V15MAGIC&&magic!=V16MAGIC&&magic!=V17MAGIC&&magic!=V18MAGIC&&magic!=V19MAGIC&&magic!=V20MAGIC&&magic!=V21MAGIC && (magic!=V22MAGIC&&(magic!=V23MAGIC&&(magic!=V24MAGIC&&(magic!=V25MAGIC&&(magic!=V26MAGIC&&(magic!=V27MAGIC&&(magic!=V28MAGIC&&magic!=V29MAGIC&&(magic!=V30MAGIC&&(magic!=V31MAGIC&&magic!=V32MAGIC))))))))))))return -1;
 zeromem(l,sizeof(*l));etch_pattern_clear(p);zeromem(s,sizeof(*s));
 etch_fx_defaults(&s->global_fx);
 l->reserved=get8(&r);get(&r,l->selection,3);
 l->valid[0]=get32(&r);l->valid[1]=get32(&r);
 get(&r,l->active,sizeof(l->active));get(&r,l->wave_names,sizeof(l->wave_names));
 uint32_t compact[2]={0,0},linear[2]={0,0};
 if(magic==V9MAGIC||(magic==V10MAGIC||magic==V11MAGIC||(magic==V12MAGIC||magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))))))))))))){compact[0]=get32(&r);compact[1]=get32(&r);linear[0]=get32(&r);linear[1]=get32(&r);}
 if((linear[0]&~compact[0])||(linear[1]&~compact[1]))return -1;
 for(uint32_t i=0;i<ETCH_WAVE_SLOTS;i++){
  if(!bit(l,i)){if((compact[i>>5]>>(i&31))&1u)return -1;continue;}
  if((linear[i>>5]>>(i&31))&1u){
   int32_t first=(int16_t)get16(&r),delta=(int32_t)get32(&r);
   if(r.error)return -1;
   for(uint32_t j=0;j<128;j++){
    int64_t value=(int64_t)first+(int64_t)delta*j;
    if(value<-32768||value>32767)return -1;
    l->slot[i][j]=(int16_t)value;
   }
  }else if((compact[i>>5]>>(i&31))&1u)wave_get_compact(&r,l->slot[i]);
  else get(&r,l->slot[i],sizeof(l->slot[i]));
 }
 if(r.error)return -1;
 uint32_t patchbits=get32(&r);
 for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if((patchbits>>i)&1u){
  get(&r,&l->patches[i],sizeof(l->patches[i]));if(!l->patches[i].valid)return -1;
 }
 get(&r,l->sequence,sizeof(l->sequence));
 s->seq_length=get16(&r);s->seq_rate=get8(&r);s->seq_gate=get8(&r);
 s->seq_swing=get8(&r);s->tempo=get8(&r);s->clock_follow=get8(&r);
 get(&r,&s->harmony,sizeof(s->harmony));get(&r,s->wave_fx,sizeof(s->wave_fx));
 uint32_t fxbit[2]={get32(&r),get32(&r)};
 for(uint32_t i=0;i<ETCH_WAVE_SLOTS;i++)if(fxbit[i>>5]&(1u<<(i&31)))get(&r,&s->slot_fx[i],sizeof(s->slot_fx[i]));
 s->patch_fx_valid=get32(&r);
 if(s->patch_fx_valid&~patchbits)return -1;
 for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if(s->patch_fx_valid&(1u<<i))get(&r,s->patch_fx[i],sizeof(s->patch_fx[i]));
 uint16_t occupied=get16(&r);
 if(r.error||occupied>ETCH_SEQ_STEPS||!validsettings(s))return -1;
 int last=-1;
 for(uint32_t i=0;i<occupied;i++){
  uint32_t step=get8(&r);uint8_t count=get8(&r),mask=get8(&r),holds=(magic==V6MAGIC||magic==V7MAGIC||magic==V8MAGIC||magic==V9MAGIC||(magic==V10MAGIC||magic==V11MAGIC||(magic==V12MAGIC||magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))))))))))))))?get8(&r):0;
  if(r.error||(int)step<=last||count>ETCH_SEQ_NOTES||holds>ETCH_SEQ_NOTES||(mask&~((1u<<ETCH_SEQ_LOCKS)-1u))||(!count&&!mask&&!holds))return -1;
  last=(int)step;
  struct etch_seq_step *st=&p->steps[step];st->count=count;st->lock_mask=mask;st->hold_count=holds;
  for(uint32_t j=0;j<count;j++){
   get(&r,&st->notes[j],sizeof(st->notes[j]));
   if(!r.error&&(st->notes[j].note>127||!st->notes[j].velocity||!st->notes[j].length))return -1;
  }
  if(magic==V8MAGIC||magic==V9MAGIC||(magic==V10MAGIC||magic==V11MAGIC||(magic==V12MAGIC||magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))))))))))))))for(uint8_t j=0;j<holds;j++){st->holds[j]=get8(&r);st->hold_ago[j]=get8(&r);}
  else get(&r,st->holds,holds);
  for(uint32_t j=0;j<holds;j++){
   if(st->holds[j]>127)return -1;
   for(uint32_t k=0;k<j;k++)if(st->holds[k]==st->holds[j]&&(st->hold_ago[k]==st->hold_ago[j]||!st->hold_ago[k]||!st->hold_ago[j]))return -1;
  }
  for(uint32_t j=0;j<ETCH_SEQ_LOCKS;j++)if(mask&(1u<<j))st->locks[j]=get16(&r);
 }
 if(magic==V4MAGIC||magic==V5MAGIC||magic==V6MAGIC||magic==V7MAGIC||magic==V8MAGIC||magic==V9MAGIC||(magic==V10MAGIC||magic==V11MAGIC||(magic==V12MAGIC||magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))))))))))))){
  s->patterns.used=get16(&r);
  if(s->patterns.used>ETCH_PATTERN_ARCHIVE_BYTES)return -1;
  for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)s->patterns.lengths[i]=get16(&r);
  if((magic==V10MAGIC||magic==V11MAGIC||(magic==V12MAGIC||magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))))))))))))){
   uint8_t mode=get8(&r);uint16_t packed=get16(&r);
   if(r.error||mode>1||packed>r.cap-r.pos||(mode==0&&packed!=s->patterns.used)||
       (mode==1&&packed>=s->patterns.used))return -1;
   if(mode==1){if(archive_lz_unpack(r.p+r.pos,packed,s->patterns.data,s->patterns.used))return -1;r.pos+=packed;}
   else get(&r,s->patterns.data,packed);
  }else get(&r,s->patterns.data,s->patterns.used);
  get(&r,s->pattern_rate,ETCH_PATCH_SLOTS);get(&r,s->pattern_gate,ETCH_PATCH_SLOTS);get(&r,s->pattern_swing,ETCH_PATCH_SLOTS);
  if(r.error||etch_pattern_bank_validate(&s->patterns))return -1;
 }
 if(magic==V5MAGIC||magic==V6MAGIC||magic==V7MAGIC||magic==V8MAGIC||magic==V9MAGIC||(magic==V10MAGIC||magic==V11MAGIC||(magic==V12MAGIC||magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))))))))))))){
  s->global_fx.chorus_rate=get16(&r);s->global_fx.chorus_depth=get16(&r);
  s->global_fx.delay_time=get16(&r);s->global_fx.delay_feedback=get16(&r);
  s->global_fx.reverb_size=get16(&r);s->global_fx.reverb_decay=get16(&r);
  s->global_fx.routing=get8(&r);
  if(s->global_fx.routing>1)return -1;
 }else etch_fx_defaults(&s->global_fx);
 if(magic==V7MAGIC||magic==V8MAGIC||magic==V9MAGIC||(magic==V10MAGIC||magic==V11MAGIC||(magic==V12MAGIC||magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))))))))))))){
  uint8_t count=get8(&r);int last=-1;
  if(count>ETCH_PATCH_SLOTS)return -1;
  for(uint8_t i=0;i<count;i++){
   uint8_t slot=get8(&r),length=get8(&r);
   if(r.error||slot>=ETCH_PATCH_SLOTS||(int)slot<=last||length<1||length>ETCH_ARP_PROGRAM_STEPS)return -1;
   last=slot;
   s->arp_programs[slot].length=length;
   get(&r,s->arp_programs[slot].steps,ETCH_ARP_PROGRAM_STEPS);
   if(!etch_arp_program_valid(&s->arp_programs[slot]))return -1;
  }
 }
 if(magic==V11MAGIC||(magic==V12MAGIC||magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))))))))))))){
  s->global_fx.ring_mix=get16(&r);s->global_fx.ring_rate=get16(&r);
  s->global_fx.ring_shape=get16(&r);s->global_fx.ring_bias=get16(&r);
  s->global_fx.eq_low=get16(&r);s->global_fx.eq_mid=get16(&r);
  s->global_fx.eq_high=get16(&r);s->global_fx.eq_hpf=get16(&r);
 }else{
  /* Older recordings had no ring/EQ fields. Existing chorus/delay/reverb
   * and project content stay exactly as they were. */
  struct etch_fx_settings defaults;etch_fx_defaults(&defaults);
  s->global_fx.ring_mix=defaults.ring_mix;s->global_fx.ring_rate=defaults.ring_rate;
  s->global_fx.ring_shape=defaults.ring_shape;s->global_fx.ring_bias=defaults.ring_bias;
  s->global_fx.eq_low=defaults.eq_low;s->global_fx.eq_mid=defaults.eq_mid;
  s->global_fx.eq_high=defaults.eq_high;s->global_fx.eq_hpf=defaults.eq_hpf;
 }
 if((magic==V12MAGIC||magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))))))))))))s->mono_mode=get8(&r);
 else s->mono_mode=0; /* legacy projects default to POLY */
 if(magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))))))))))){
  s->preset_valid=get32(&r);
  if(r.error||s->preset_valid&~patchbits)return -1;
  for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if(s->preset_valid&(1u<<i)){
   etch_fx_defaults(&s->preset_fx[i]);
   get(&r,&s->preset_fx[i],ETCH_FX_V13_SIZE);
   if(r.error||s->preset_fx[i].routing>1||s->preset_fx[i].ring_mix>32767||
      s->preset_fx[i].eq_low>32767||s->preset_fx[i].eq_mid>32767||s->preset_fx[i].eq_high>32767)return -1;
   for(uint32_t j=0;j<3;j++){
    uint8_t method=get8(&r);
    if(method>2||r.error)return -1;
    int16_t *wave=s->preset_wave[i][j];
    if(method==2){
     int32_t first=(int16_t)get16(&r),delta=(int32_t)get32(&r);
     if(r.error)return -1;
     for(uint32_t k=0;k<128;k++){
      int64_t value=(int64_t)first+(int64_t)delta*k;
      if(value<-32768||value>32767)return -1;
      wave[k]=(int16_t)value;
     }
    }else if(method==1)wave_get_compact(&r,wave);
    else get(&r,wave,sizeof(s->preset_wave[i][j]));
   }
  }
 }
 if((magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))))))))))){
  fx14_get(&r,&s->global_fx);
  for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if(s->preset_valid&(1u<<i))fx14_get(&r,&s->preset_fx[i]);
  if(r.error||!fx14_valid(&s->global_fx))return -1;
  for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if((s->preset_valid&(1u<<i))&&!fx14_valid(&s->preset_fx[i]))return -1;
 }
 if(magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))))))))))){
  s->global_fx.tape_wow=get16(&r);s->global_fx.tape_flutter=get16(&r);
  s->global_fx.tape_age=get16(&r);s->global_fx.tape_mix=get16(&r);
  for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if(s->preset_valid&(1u<<i)){
   struct etch_fx_settings *fx=&s->preset_fx[i];
   fx->tape_wow=get16(&r);fx->tape_flutter=get16(&r);
   fx->tape_age=get16(&r);fx->tape_mix=get16(&r);
  }
  if(r.error||!fx15_valid(&s->global_fx))return -1;
  for(uint32_t i=0;i<ETCH_PATCH_SLOTS;i++)if((s->preset_valid&(1u<<i))&&!fx15_valid(&s->preset_fx[i]))return -1;
 }
 if((magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))))))))))){
  struct etch_fx_settings *fx=&s->global_fx;
  for(uint32_t i=0;i<16;i++)((&fx->reverse_time)[i])=get16(&r);
  for(uint32_t slot=0;slot<ETCH_PATCH_SLOTS;slot++)if(s->preset_valid&(1u<<slot)){
   fx=&s->preset_fx[slot];for(uint32_t i=0;i<16;i++)((&fx->reverse_time)[i])=get16(&r);
  }
  if(r.error)return -1;
  for(uint32_t i=0;i<16;i++)if((&s->global_fx.reverse_time)[i]>32767)return -1;
  for(uint32_t slot=0;slot<ETCH_PATCH_SLOTS;slot++)if(s->preset_valid&(1u<<slot))
   for(uint32_t i=0;i<16;i++)if((&s->preset_fx[slot].reverse_time)[i]>32767)return -1;
 }
 if((magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))))))))){
  struct etch_fx_settings *settings=&s->global_fx;
  for(uint32_t slot=0;slot<=ETCH_PATCH_SLOTS;slot++){
   if(slot){if(!(s->preset_valid&(1u<<(slot-1u))))continue;settings=&s->preset_fx[slot-1u];}
   for(uint32_t wave=0;wave<2u;wave++)for(uint32_t insert=0;insert<2u;insert++){
    settings->pre[wave][insert]=get16(&r);
    if(etch_pre_type(settings,(uint8_t)wave,(uint8_t)insert)>5u)return -1;
   }
   uint16_t packed=get16(&r);
   if(magic!=V20MAGIC && magic!=V21MAGIC && (magic!=V22MAGIC&&(magic!=V23MAGIC&&(magic!=V24MAGIC&&(magic!=V25MAGIC&&(magic!=V26MAGIC&&(magic!=V27MAGIC&&(magic!=V28MAGIC&&magic!=V29MAGIC&&(magic!=V30MAGIC&&(magic!=V31MAGIC&&magic!=V32MAGIC))))))))) && (packed&7u)>6u)return -1;
   settings->c_fx=(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))))))?packed:etch_c_fx_migrate_old(packed);
  }
 }
 if((magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))))))))){
  s->motion.length=get8(&r);s->motion.mask=get8(&r);
  if(r.error||s->motion.length>ETCH_MOTION_STEPS||s->motion.mask>15u)return -1;
  for(uint32_t k=0;k<4u;k++)for(uint32_t i=0;i<s->motion.length;i++)
   s->motion.lane[k][i]=get16(&r);
  if(r.error||!etch_motion_valid(&s->motion))return -1;
 }
 if((magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))))))){
  s->evolution.depth=get8(&r);s->evolution.rate=get8(&r);
  s->evolution.mode=get8(&r);s->evolution.sync=get8(&r);
  if(r.error||!etch_evolution_valid(&s->evolution))return -1;
 }
 if((magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))))))for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++){
  uint16_t v=get16(&r);
  if(s==riff_owner&&riff_data)riff_data[i]=v;
 }else if(s==riff_owner&&riff_data)for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++)riff_data[i]=0;
 if((magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))))){
  s->organic.drift=get8(&r);s->organic.rate=get8(&r);
  s->organic.phase=get8(&r);s->organic.variance=get8(&r);
  if(r.error||!etch_organic_valid(&s->organic))return -1;
 }
 if((magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))))){
  s->living.depth=get8(&r);s->living.rate=get8(&r);
  s->living.color=get8(&r);s->living.vary=get8(&r);
  if(r.error||!etch_living_valid(&s->living))return -1;
 }
 if((magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))){
  for(uint32_t i=0;i<ETCH_MATRIX_ROUTES;i++){
   s->matrix.route[i].source=get8(&r);s->matrix.route[i].target=get8(&r);s->matrix.route[i].amount=get8(&r);
  }
  if(r.error||!etch_matrix_valid(&s->matrix)||
     ((magic!=V30MAGIC&&(magic!=V31MAGIC&&magic!=V32MAGIC))&&!etch_matrix_valid_legacy(&s->matrix)))return -1;
 }
 if((magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))))))){
  for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++){
   uint16_t variant=get16(&r);uint8_t mutation=get8(&r),density=get8(&r);
   if(mutation>100u||density>100u)return -1;
   if(s==riff2_owner&&riff2_data){riff2_data->riff_variants[i]=variant;riff2_data->riff_mutations[i]=mutation;riff2_data->riff_densities[i]=density;}
  }
 }else if(s==riff2_owner&&riff2_data)for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++){
  riff2_data->riff_variants[i]=0;riff2_data->riff_mutations[i]=0;riff2_data->riff_densities[i]=50u;
 }
 if(r.error||!validsettings(s))return -1;
 if(magic==V8MAGIC||magic==V9MAGIC||(magic==V10MAGIC||magic==V11MAGIC||(magic==V12MAGIC||magic==V13MAGIC||(magic==V14MAGIC||magic==V15MAGIC||(magic==V16MAGIC||(magic==V17MAGIC||(magic==V18MAGIC||(magic==V19MAGIC||(magic==V20MAGIC||(magic==V21MAGIC||(magic==V22MAGIC||(magic==V23MAGIC||(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))))))))))))))for(uint16_t step=0;step<ETCH_SEQ_STEPS;step++){
  const struct etch_seq_step *st=&p->steps[step];
  for(uint8_t k=0;k<st->hold_count;k++)if(st->hold_ago[k]){
   uint8_t source=(uint8_t)(step-st->hold_ago[k]);uint8_t found=0;
   const struct etch_seq_step *srcst=&p->steps[source];
   for(uint8_t j=0;j<srcst->count;j++)if(srcst->notes[j].note==st->holds[k])found=1;
   if(!found)return -1;
  }
 }
 if((magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))){
  struct etch_preset_dsp_bank *bank=etch_dsp_bound(s);
  if(!bank)return -1;
  for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++){
   struct etch_preset_dsp *v=&bank->slot[i];
   get(&r,v,(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))?sizeof(*v):(magic==V27MAGIC)?32u:30u);
   if((magic!=V28MAGIC&&magic!=V29MAGIC&&(magic!=V30MAGIC&&(magic!=V31MAGIC&&magic!=V32MAGIC))))v->velocity_curve=0u;
   if(magic==V26MAGIC){v->spectral_gain=50u;v->spectral_tilt=50u;}
   if(r.error||!etch_organic_valid(&v->organic)||!etch_living_valid(&v->living)||
     ((magic!=V30MAGIC&&(magic!=V31MAGIC&&magic!=V32MAGIC))&&!etch_matrix_valid_legacy(&v->matrix))||
     !etch_matrix_valid(&v->matrix)||!etch_evolution_valid(&v->evolution)||
     (magic==V32MAGIC?((v->spectral_wave&0xf0u)!=0u || (v->spectral_wave&3u)>2u):v->spectral_wave>2u)||v->spectral_part>32u||v->blend_a>=32u||
     v->blend_b>=32u||v->blend_pct>100u||v->blend_on>1u||
     v->spectral_gain>100u||v->spectral_tilt>100u|| (magic==V32MAGIC?(v->velocity_curve&3u)>2u:v->velocity_curve>2u))return -1;
  }
 }
 {
  struct etch_preset_dsp_bank *bank=etch_dsp_bound(s);
  if((magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC)))){
   if(!bank)return -1;
   uint8_t packed[8];get(&r,packed,8u);
   if(r.error||!etch_key_groups_unpack(&bank->groups,packed))return -1;
  }else if(bank)etch_key_groups_init(&bank->groups);
 }
 {
  struct etch_preset_dsp_bank *bank=etch_dsp_bound(s);
  if((magic==V31MAGIC||magic==V32MAGIC)){
   if(!bank)return -1;
   get(&r,&bank->performance,sizeof(bank->performance));
   if(r.error||!etch_perf_valid(&bank->performance))return -1;
  }else if(bank)etch_perf_init(&bank->performance);
 }
 if(r.error||r.pos!=len)return -1;
 s->source_version=magic==V32MAGIC?32:magic==V31MAGIC?31:magic==V30MAGIC?30:magic==V29MAGIC?29:(magic==V28MAGIC)?28:(magic==V27MAGIC)?27:magic==V26MAGIC?26:(magic==V25MAGIC)?25:(magic==V24MAGIC||(magic==V25MAGIC||(magic==V26MAGIC||(magic==V27MAGIC||(magic==V28MAGIC||(magic==V29MAGIC||(magic==V30MAGIC||(magic==V31MAGIC||magic==V32MAGIC))))))))?24:magic==V23MAGIC?23:magic==V22MAGIC?22:magic==V21MAGIC?21:magic==V20MAGIC?20:magic==V19MAGIC?19:magic==V18MAGIC?18:magic==V17MAGIC?17:magic==V16MAGIC?16:magic==V15MAGIC?15:magic==V14MAGIC?14:magic==V13MAGIC?13:magic==V12MAGIC?12:magic==V11MAGIC?11:magic==V10MAGIC?10:magic==V9MAGIC?9:magic==V8MAGIC?8:magic==V7MAGIC?7:magic==V6MAGIC?6:magic==V5MAGIC?5:magic==V4MAGIC?4:3;
 for(uint32_t w=0;w<3;w++)if(l->selection[w]>=ETCH_WAVE_SLOTS)return -1;
 return 0;
}
void etch_project_legacy(const struct etch_wave_library *l,struct etch_native_pattern *p,struct etch_project_settings *s){
 if(p){etch_pattern_clear(p);if(l){
  uint8_t nonzero=0;for(uint32_t i=0;i<64;i++)if(l->sequence[i]){nonzero=1;break;}
  if(nonzero)for(uint32_t i=0;i<64;i++)if(l->sequence[i]<128)etch_seq_put(p,(uint16_t)i,l->sequence[i],127,1);
 }}
 if(s){zeromem(s,sizeof(*s));s->seq_length=64;s->seq_gate=75;s->tempo=120;s->source_version=2;etch_fx_defaults(&s->global_fx);}
}

int etch_project_measure(const struct etch_wave_library *l,const struct etch_native_pattern *p,
 const struct etch_project_settings *s,uint32_t *required){
 if(!required)return -1;
 return etch_project_encode(l,p,s,0,UINT32_MAX,required);
}
