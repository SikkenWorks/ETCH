/* SPDX-License-Identifier: GPL-3.0-only */
#include "native_seq_ui.h"
uint16_t etch_seq_grid_key_select(uint16_t current,uint16_t length,uint8_t key){
 if(!length||length>ETCH_SEQ_STEPS||key>=16)return current<length?current:0;
 uint16_t base=(uint16_t)((current/16u)*16u);
 uint16_t dest=(uint16_t)(base+key);
 return dest<length?dest:(uint16_t)(length-1u);
}
int etch_seq_grid_view(const struct etch_native_pattern *p,uint16_t length,
                       uint16_t selected,struct etch_seq_grid_view *out){
 if(!p||!out||!length||length>ETCH_SEQ_STEPS||selected>=length)return -1;
 struct etch_seq_grid_view v={0,0,0,0};
 uint16_t base=(uint16_t)((selected/16u)*16u);
 for(uint8_t i=0;i<16&&(uint16_t)(base+i)<length;i++){
  const struct etch_seq_step *s=&p->steps[base+i];
  if(s->count>ETCH_SEQ_NOTES||s->hold_count>ETCH_SEQ_NOTES)return -1;
  if(s->count)v.triggers|=(uint16_t)(1u<<i);
  struct etch_step_layers layers;
  etch_seq_layers(p,length,(uint16_t)(base+i),&layers);
  if(layers.hold_pitches)v.holds|=(uint16_t)(1u<<i);
  if(s->lock_mask)v.locks|=(uint16_t)(1u<<i);
 }
 v.selected=(uint16_t)(1u<<(selected-base));*out=v;return 0;
}
uint8_t etch_seq_note_cursor(uint8_t count,uint8_t cursor){
 if(!count)return 0;
 return cursor<count?cursor:(uint8_t)(count-1u);
}
static uint32_t keybit(uint8_t midi,int8_t octave){
 int32_t k=(int32_t)midi-53-12*(int32_t)octave;
 return k>=0&&k<27?(uint32_t)1u<<(uint32_t)k:0u;
}
int etch_seq_key_view(const struct etch_native_pattern *p,uint16_t step,
                     int8_t octave,uint8_t cursor,struct etch_seq_key_view *out){
 if(!p||!out||step>=ETCH_SEQ_STEPS)return -1;
 const struct etch_seq_step *s=&p->steps[step];
 if(s->count>ETCH_SEQ_NOTES||s->hold_count>ETCH_SEQ_NOTES)return -1;
 struct etch_seq_key_view view={0,0,0};
 for(uint8_t i=0;i<s->count;i++)if(s->notes[i].note<=127)
  view.triggers|=keybit(s->notes[i].note,octave);
 for(uint8_t i=0;i<s->hold_count;i++)if(s->holds[i]<=127)
  view.holds|=keybit(s->holds[i],octave);
 if(s->count)view.selected=keybit(s->notes[etch_seq_note_cursor(s->count,cursor)].note,octave);
 *out=view;
 return 0;
}
uint8_t etch_seq_step_clear(struct etch_native_pattern *p,uint16_t step,uint8_t mask){
 if(!p||step>=ETCH_SEQ_STEPS||!mask||(mask&~7u))return 0;
 struct etch_seq_step *s=&p->steps[step];
 if(s->count>ETCH_SEQ_NOTES||s->hold_count>ETCH_SEQ_NOTES)return 0;
 if(mask&1u)for(uint8_t i=0;i<s->count;i++){
  const uint8_t pitch=s->notes[i].note;
  for(uint16_t st=0;st<ETCH_SEQ_STEPS;st++){
   const struct etch_seq_step *q=&p->steps[st];
   for(uint8_t j=0;j<q->hold_count;j++)if(q->holds[j]==pitch&&q->hold_ago[j]&&
      (uint8_t)(st-q->hold_ago[j])==(uint8_t)step)return 0;
  }
 }
 if(mask&1u){s->count=0;for(uint8_t i=0;i<ETCH_SEQ_NOTES;i++)s->notes[i]=(struct etch_seq_note){0,0,0};}
 if(mask&2u){s->hold_count=0;for(uint8_t i=0;i<ETCH_SEQ_NOTES;i++){s->holds[i]=0;s->hold_ago[i]=0;}}
 if(mask&4u){s->lock_mask=0;for(uint8_t i=0;i<ETCH_SEQ_LOCKS;i++)s->locks[i]=0;}
 return 1;
}

uint8_t etch_seq_note_key_select(struct etch_native_pattern *p,uint16_t step,
 uint8_t pitch,uint8_t *cursor){
 if(!p||!cursor||step>=ETCH_SEQ_STEPS||pitch>127)return 0;
 struct etch_seq_step *s=&p->steps[step];
 if(s->count>ETCH_SEQ_NOTES)return 0;
 for(uint8_t i=0;i<s->count;i++)if(s->notes[i].note==pitch){*cursor=i;return (uint8_t)(i+1u);}
 if(!etch_seq_put(p,step,pitch,127,1))return 0;
 *cursor=(uint8_t)(s->count-1u);return s->count;
}
