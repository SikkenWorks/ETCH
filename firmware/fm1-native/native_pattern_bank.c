/* SPDX-License-Identifier: GPL-3.0-only */
#include "native_pattern_bank.h"
static struct etch_native_pattern etch_pattern_bank_tmp; /* main loop only; never audio callback */
static uint16_t get16(const uint8_t *s){return (uint16_t)s[0]|((uint16_t)s[1]<<8);}
static void put16(uint8_t *s,uint16_t n){s[0]=(uint8_t)n;s[1]=(uint8_t)(n>>8);}
static int unpack(const uint8_t *p,uint16_t size,struct etch_native_pattern *out){
 if(size<2)return -1;
 uint16_t header=get16(p),count=header&0x3fffu,at=2;uint8_t v2=(header&0x8000u)!=0;uint8_t v3=(header&0x4000u)!=0;
 if(v3&&!v2)return -1;int32_t last=-1;
 if(count>ETCH_SEQ_STEPS)return -1;
 etch_pattern_clear(&etch_pattern_bank_tmp);
 for(uint16_t i=0;i<count;i++){
  if((uint32_t)at+(v2?4u:3u)>size)return -1;
  uint8_t step=p[at++],notes=p[at++],mask=p[at++],holds=v2?p[at++]:0;
  if((int32_t)step<=last||notes>ETCH_SEQ_NOTES||holds>ETCH_SEQ_NOTES||(mask&~63u)||(!notes&&!mask&&!holds))return -1;
  last=step;
  struct etch_seq_step *s=&etch_pattern_bank_tmp.steps[step];s->count=notes;s->lock_mask=mask;s->hold_count=holds;
  if((uint32_t)at+(uint32_t)notes*3u+holds>size)return -1;
  for(uint8_t k=0;k<notes;k++){
   s->notes[k].note=p[at++];s->notes[k].velocity=p[at++];s->notes[k].length=p[at++];
   if(s->notes[k].note>127||!s->notes[k].velocity||!s->notes[k].length)return -1;
  }
  if((uint32_t)at+holds*(v3?2u:1u)>size)return -1;
  for(uint8_t k=0;k<holds;k++){
   uint8_t pitch=p[at++];uint8_t ago=v3?p[at++]:0;
   if(pitch>127)return -1;
   for(uint8_t j=0;j<k;j++)if(s->holds[j]==pitch&&(s->hold_ago[j]==ago||!s->hold_ago[j]||!ago))return -1;
   s->holds[k]=pitch;s->hold_ago[k]=ago;
  }
  for(uint8_t k=0;k<ETCH_SEQ_LOCKS;k++)if(mask&(1u<<k)){
   if((uint32_t)at+2>size)return -1;
   s->locks[k]=get16(p+at);at+=2;
  }
 }
 if(at!=size)return -1;
 if(v3)for(uint16_t step=0;step<ETCH_SEQ_STEPS;step++){
  const struct etch_seq_step *s=&etch_pattern_bank_tmp.steps[step];
  for(uint8_t i=0;i<s->hold_count;i++)if(s->hold_ago[i]){
   uint8_t owner=(uint8_t)(step-s->hold_ago[i]);uint8_t ok=0;
   const struct etch_seq_step *source=&etch_pattern_bank_tmp.steps[owner];
   for(uint8_t j=0;j<source->count;j++)if(source->notes[j].note==s->holds[i])ok=1;
   if(!ok)return -1;
  }
 }
 *out=etch_pattern_bank_tmp;return 0;
}
/* A slot is stored in ascending order; every nonempty record is independently parseable. */
int etch_pattern_bank_validate(const struct etch_pattern_bank *b){
 if(!b||b->used>ETCH_PATTERN_ARCHIVE_BYTES)return -1;
 uint32_t offset=0;
 for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++){
  uint16_t len=b->lengths[i];if(len){
   if((uint32_t)len+offset>b->used||unpack(b->data+offset,len,&etch_pattern_bank_tmp))return -1;
   offset+=len;
  }
 }
 return offset==b->used?0:-1;
}
void etch_pattern_bank_clear(struct etch_pattern_bank *b){
 if(!b)return;
 for(uint8_t i=0;i<ETCH_PATCH_SLOTS;i++)b->lengths[i]=0;
 b->used=0;
}
int etch_pattern_bank_fetch(const struct etch_pattern_bank *b,uint8_t slot,struct etch_native_pattern *out){
 if(!b||!out||slot>=ETCH_PATCH_SLOTS)return -1;
 uint32_t off=0;
 for(uint8_t i=0;i<slot;i++)off+=b->lengths[i];
 uint16_t len=b->lengths[slot];
 if(off+len>b->used||b->used>ETCH_PATTERN_ARCHIVE_BYTES)return -1;
 if(!len){etch_pattern_clear(out);return 0;}
 return unpack(b->data+off,len,out)?-1:1;
}
int etch_pattern_bank_capture(struct etch_pattern_bank *b,uint8_t slot,const struct etch_native_pattern *p){
 if(!b||!p||slot>=ETCH_PATCH_SLOTS||b->used>ETCH_PATTERN_ARCHIVE_BYTES)return -1;
 uint32_t off=0;for(uint8_t i=0;i<slot;i++)off+=b->lengths[i];
 uint16_t old=b->lengths[slot];if(off+old>b->used)return -1;
 uint32_t occupied=0,bytes=2;
 for(uint32_t step=0;step<ETCH_SEQ_STEPS;step++){
  const struct etch_seq_step *s=&p->steps[step];
  if(s->count>ETCH_SEQ_NOTES||s->hold_count>ETCH_SEQ_NOTES||(s->lock_mask&~63u))return -1;
  if(s->count||s->lock_mask||s->hold_count){
   occupied++;bytes+=4u+(uint32_t)s->count*3u+s->hold_count*2u;
   for(uint8_t k=0;k<ETCH_SEQ_LOCKS;k++)if(s->lock_mask&(1u<<k))bytes+=2;
  }
  for(uint8_t k=0;k<s->count;k++)if(s->notes[k].note>127||!s->notes[k].velocity||!s->notes[k].length)return -1;
  for(uint8_t k=0;k<s->hold_count;k++){
   if(s->holds[k]>127)return -1;
   if(s->hold_ago[k]){
    uint8_t owner=(uint8_t)(step-s->hold_ago[k]);const struct etch_seq_step *src=&p->steps[owner];uint8_t yes=0;
    for(uint8_t t=0;t<src->count;t++)if(src->notes[t].note==s->holds[k])yes=1;
    if(!yes)return -1;
   }
   for(uint8_t j=0;j<k;j++)if(s->holds[k]==s->holds[j]&&(s->hold_ago[k]==s->hold_ago[j]||!s->hold_ago[k]||!s->hold_ago[j]))return -1;
  }
 }
 if(!occupied)bytes=0; /* Truly empty patterns do not consume flash space. */
 if((uint32_t)b->used-old+bytes>ETCH_PATTERN_ARCHIVE_BYTES)return -2;
 /* All validation and sizing happen before mutation. */
 uint32_t suffix=(uint32_t)b->used-off-old;
 if(bytes>old){for(uint32_t j=suffix;j>0;j--)b->data[off+bytes+j-1]=b->data[off+old+j-1];}
 else if(bytes<old){for(uint32_t j=0;j<suffix;j++)b->data[off+bytes+j]=b->data[off+old+j];}
 b->lengths[slot]=(uint16_t)bytes;b->used=(uint16_t)((uint32_t)b->used-old+bytes);
 if(!bytes)return 0;
 uint32_t at=off;put16(b->data+at,(uint16_t)(occupied|0xc000u));at+=2;
 for(uint32_t step=0;step<ETCH_SEQ_STEPS;step++){
  const struct etch_seq_step *s=&p->steps[step];if(!s->count&&!s->lock_mask&&!s->hold_count)continue;
  b->data[at++]=(uint8_t)step;b->data[at++]=s->count;b->data[at++]=s->lock_mask;b->data[at++]=s->hold_count;
  for(uint8_t k=0;k<s->count;k++){b->data[at++]=s->notes[k].note;b->data[at++]=s->notes[k].velocity;b->data[at++]=s->notes[k].length;}
  for(uint8_t k=0;k<s->hold_count;k++){b->data[at++]=s->holds[k];b->data[at++]=s->hold_ago[k];}
  for(uint8_t k=0;k<ETCH_SEQ_LOCKS;k++)if(s->lock_mask&(1u<<k)){put16(b->data+at,s->locks[k]);at+=2;}
 }
 return at==off+bytes?0:-1;
}
