/* SPDX-License-Identifier: GPL-3.0-only */
#include "native_performance.h"
static const uint8_t intervals[9][7]={
 {0,2,4,5,7,9,11},{0,2,3,5,7,9,10},{0,1,3,5,7,8,10},
 {0,2,4,6,7,9,11},{0,2,4,5,7,9,10},{0,2,3,5,7,8,10},
 {0,1,3,5,6,8,10},{0,2,3,5,7,8,11},{0,2,3,5,7,9,11}
};
/* K4 generates a repeatable sixteen-step melodic riff, with occasional rests,
 * small scale-degree motions and a few octave accents. No randomness in the
 * audio callback, no heap, no reliance on KEYLOCK being enabled. */
uint32_t etch_riff_regenerate(uint32_t previous){
 uint32_t x=previous?previous:0x13579bdfu;
 x^=x<<13;x^=x>>17;x^=x<<5;
 return (uint16_t)(x?x:0x9e3779b9u);
}
static uint8_t riff_allowed(uint8_t pitch,const struct etch_harmony_settings *h){
 uint8_t root=h?h->root%12u:0u,mode=h&&h->mode<9u?h->mode:0u;
 uint8_t pc=(uint8_t)((pitch+12u-root)%12u);
 for(uint8_t k=0;k<7u;k++)if(pc==intervals[mode][k])return 1u;
 return 0u;
}
uint8_t etch_riff_pick(uint32_t seed,uint8_t step,uint8_t base,const struct etch_harmony_settings *h,uint8_t octaves){
 if(base>127u)return 255u;
 uint32_t x=(seed?seed:0x13579bdfu)^((uint32_t)(step&15u)*0x9e3779b9u);
 x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;
 /* Downbeats always sound; interior offbeats may rest. */
 if((step&3u)!=0u && ((x>>12)&7u)==0u)return 255u;
 int32_t start=base;
 while(start<127&&!riff_allowed((uint8_t)start,h))++start;
 if(start==127&&!riff_allowed((uint8_t)start,h)){
  while(start>0&&!riff_allowed((uint8_t)start,h))--start;
 }
 /* Keep pitches within 1 octave either way; tonic-biased walking intervals. */
 static const int8_t degrees[16]={0,0,1,2,-1,3,0,-2,1,4,-1,2,0,-3,1,5};
 int8_t moves=degrees[(x>>20)&15u];
 if(!(step&3u))moves=0; /* anchor the riff rhythmically */
 int32_t n=start,remaining=moves<0?-moves:moves;
 while(remaining--){
  do{n+=moves<0?-1:1;}while(n>0&&n<127&&!riff_allowed((uint8_t)n,h));
  if(n<0)n=0;if(n>127)n=127;
 }
 /* +/- octave spread follows the ordinary ARP OCT knob. */
 uint8_t range=octaves>=5u?octaves-4u:octaves;
 if(!range||range>4u)range=1u;
 int32_t first=octaves>=5u?1-(int32_t)range:0;
 int32_t shift=(first+(int32_t)(((step>>2)+(seed&3u))%range))*12;
 n+=shift;
 if(n<0)n=0;if(n>127)n=127;
 /* Clamp boundary notes back into the scale. */
 while(n<127&&!riff_allowed((uint8_t)n,h))++n;
 while(n>0&&!riff_allowed((uint8_t)n,h))--n;
 return (uint8_t)n;
}
int etch_arp_program_valid(const struct etch_arp_program *p){
 if(!p||p->length>ETCH_ARP_PROGRAM_STEPS)return 0;
 for(uint32_t i=0;i<p->length;i++)if(p->steps[i]!=255&&p->steps[i]>31)return 0;
 return 1;
}
void etch_arp_program_init(struct etch_arp_program *p){
 if(!p)return;p->length=16;
 for(uint8_t i=0;i<16;i++)p->steps[i]=i;
}
uint8_t etch_arp_program_pick(const struct etch_arp_program *p,uint32_t step,uint32_t held_total){
 if(!held_total)return 255;
 if(!p||!p->length||!etch_arp_program_valid(p))return (uint8_t)(step%held_total);
 uint8_t value=p->steps[step%p->length];
 return value==255?255:(uint8_t)(value%held_total);
}
uint8_t etch_arp_pack_flags(uint8_t rate,uint8_t octaves,uint8_t gate_index){
 if(rate>3)rate=2;
 if(octaves<1||octaves>6)octaves=1;
 if(gate_index>3)gate_index=2;
 return (uint8_t)(0x80u|rate|(((octaves>=5u?octaves-5u:octaves-1u)&3u)<<2)|(gate_index<<4)|(octaves>=5u?0x40u:0u));
}
void etch_arp_unpack_flags(uint8_t flags,uint8_t *rate,uint8_t *octaves,uint8_t *gate_index){
 if(rate)*rate=flags&0x80u?(uint8_t)(flags&3u):2u;
 if(octaves)*octaves=flags&0x80u?(uint8_t)((flags&0x40u)?5u+((flags>>2)&1u):1u+((flags>>2)&3u)):1u;
 if(gate_index)*gate_index=flags&0x80u?(uint8_t)((flags>>4)&3u):2u;
}
void etch_pattern_clear(struct etch_native_pattern *p){
 if(!p)return;
 uint8_t *b=(uint8_t*)p;for(uint32_t i=0;i<sizeof(*p);i++)b[i]=0;
}
uint8_t etch_seq_put(struct etch_native_pattern *p,uint16_t step,uint8_t note,uint8_t velocity,uint8_t length){
 if(!p||step>=ETCH_SEQ_STEPS||note>127||!length||!velocity)return 0;
 struct etch_seq_step *s=&p->steps[step];
 for(uint8_t i=0;i<s->count;i++)if(s->notes[i].note==note){s->notes[i].velocity=velocity;s->notes[i].length=length;return 1;}
 if(s->count>=ETCH_SEQ_NOTES)return 0;
 struct etch_seq_note *n=&s->notes[s->count++];n->note=note;n->velocity=velocity;n->length=length;return 1;
}
/* Do not break V8 references when a trigger is deleted or retuned.
 * Never leave a linked HOLD pointing to an absent source note. */
static uint8_t has_linked_hold(const struct etch_native_pattern *p,uint16_t source,uint8_t pitch){
 for(uint16_t st=0;st<ETCH_SEQ_STEPS;st++){
  const struct etch_seq_step *s=&p->steps[st];
  for(uint8_t i=0;i<s->hold_count&&i<ETCH_SEQ_NOTES;i++)if(s->holds[i]==pitch&&
    s->hold_ago[i]&&(uint8_t)(st-s->hold_ago[i])==(uint8_t)source)return 1;
 }
 return 0;
}
uint8_t etch_seq_note_edit(struct etch_native_pattern *p,uint16_t step,uint8_t index,
 uint8_t note,uint8_t velocity,uint8_t length){
 if(!p||step>=ETCH_SEQ_STEPS||note>127||!velocity||!length)return 0;
 struct etch_seq_step *s=&p->steps[step];
 if(s->count>ETCH_SEQ_NOTES||index>=s->count)return 0;
 for(uint8_t j=0;j<s->count;j++)if(j!=index&&s->notes[j].note==note)return 2;
 if(s->notes[index].note!=note&&has_linked_hold(p,step,s->notes[index].note))return 3;
 s->notes[index].note=note;
 s->notes[index].velocity=velocity;
 s->notes[index].length=length;
 return 1;
}
uint8_t etch_seq_note_remove(struct etch_native_pattern *p,uint16_t step,uint8_t index){
 if(!p||step>=ETCH_SEQ_STEPS)return 0;
 struct etch_seq_step *s=&p->steps[step];
 if(s->count>ETCH_SEQ_NOTES||index>=s->count)return 0;
 if(has_linked_hold(p,step,s->notes[index].note))return 2;
 for(uint8_t j=index;j+1<s->count;j++)s->notes[j]=s->notes[j+1];
 --s->count;s->notes[s->count]=(struct etch_seq_note){0,0,0};
 return 1;
}
uint16_t etch_seq_grid_move(uint16_t current,uint16_t length,int32_t pages,
 int32_t columns,int32_t rows,int32_t select){
 if(!length||length>ETCH_SEQ_STEPS)return 0;
 if(current>=length)current=(uint16_t)(length-1);
 /* Each encoder delta is capped before multiplying so corrupt input cannot
  * overflow the navigation accumulator. */
 if(pages>256)pages=256;if(pages< -256)pages=-256;
 if(columns>256)columns=256;if(columns< -256)columns=-256;
 if(rows>256)rows=256;if(rows< -256)rows=-256;
 if(select>256)select=256;if(select< -256)select=-256;
 int32_t target=(int32_t)current+pages*16+columns+rows*8+select;
 if(target<0)target=0;
 if(target>=(int32_t)length)target=(int32_t)length-1;
 return (uint16_t)target;
}
static int allowed(uint8_t note,const struct etch_harmony_settings *h){
 uint8_t root=h->root%12,mode=h->mode<9?h->mode:0,pc=(uint8_t)((note+12-root)%12);
 for(uint8_t k=0;k<7;k++)if(pc==intervals[mode][k])return 1;
 return 0;
}
uint8_t etch_quantize_note(uint8_t note,const struct etch_harmony_settings *h){
 if(!h||h->keylock!=2||note>127)return note;
 if(allowed(note,h))return note;
 for(uint8_t delta=1;delta<=6;delta++){
  if(note>=delta&&allowed((uint8_t)(note-delta),h))return (uint8_t)(note-delta);
  if(note+delta<=127&&allowed((uint8_t)(note+delta),h))return (uint8_t)(note+delta);
 }return note;
}
uint8_t etch_chord_notes(uint8_t note,const struct etch_harmony_settings *h,uint8_t out[4]){
 if(!out||note>127)return 0;
 if(!h||h->chord==0){out[0]=etch_quantize_note(note,h);return 1;}
 struct etch_harmony_settings lock=*h;lock.keylock=2;
 uint8_t root=etch_quantize_note(note,&lock),mode=h->mode<9?h->mode:0,pc=(uint8_t)((root+12-h->root%12)%12);
 uint8_t degree=0;while(degree<7&&intervals[mode][degree]!=pc)degree++;
 if(degree==7){out[0]=root;return 1;}
 uint8_t size=h->chord==2?4:3;int16_t min=127,max=0,raw[4];
 for(uint8_t k=0;k<size;k++){
  uint8_t pos=(uint8_t)(degree+k*2),index=(uint8_t)(pos%7),oct=(uint8_t)(pos/7);
  raw[k]=(int16_t)root+intervals[mode][index]+oct*12-intervals[mode][degree];
  if(raw[k]<min)min=raw[k];
  if(raw[k]>max)max=raw[k];
 }
 int16_t shift=0;while(max+shift>127)shift-=12;while(min+shift<0)shift+=12;
 for(uint8_t k=0;k<size;k++)out[k]=(uint8_t)(raw[k]+shift);
 return size;
}

uint8_t etch_seq_toggle(struct etch_native_pattern *p,uint16_t step,uint8_t note,uint8_t velocity,uint8_t length){
 if(!p||step>=ETCH_SEQ_STEPS||note>127||!velocity||!length)return 0;
 struct etch_seq_step *s=&p->steps[step];
 for(uint8_t i=0;i<s->count;i++)if(s->notes[i].note==note){
  return etch_seq_note_remove(p,step,i)==1?2:0;
 }
 return etch_seq_put(p,step,note,velocity,length);
}
void etch_seq_layers(const struct etch_native_pattern *p,uint16_t length,uint16_t step,struct etch_step_layers *out){
 if(!out)return;
 uint8_t *bytes=(uint8_t*)out;
 for(uint32_t i=0;i<sizeof(*out);i++)bytes[i]=0;
 if(!p||!length||length>ETCH_SEQ_STEPS||step>=length)return;
 const struct etch_seq_step *here=&p->steps[step];
 for(uint8_t i=0;i<here->count&&i<ETCH_SEQ_NOTES;i++){
  uint8_t pitch=here->notes[i].note;
  if(pitch>127||!here->notes[i].velocity)continue;
  out->trigger_bits[pitch>>3]|=(uint8_t)(1u<<(pitch&7));
  ++out->trigger_count;
 }
 /* Authored HOLD may be visible without an upstream attack; it never generates an attack. */
 for(uint8_t i=0;i<here->hold_count&&i<ETCH_SEQ_NOTES;i++){
  uint8_t pitch=here->holds[i];if(pitch<=127)
   out->hold_bits[pitch>>3]|=(uint8_t)(1u<<(pitch&7));
 }
 for(uint16_t back=1;back<length;back++){
  uint16_t earlier=(uint16_t)((step+length-back)%length);
  const struct etch_seq_step *before=&p->steps[earlier];
  for(uint8_t i=0;i<before->count&&i<ETCH_SEQ_NOTES;i++){
   const struct etch_seq_note *n=&before->notes[i];
   if(n->note>127||!n->velocity)continue;
   if(n->length<=back && (!here->hold_count||etch_seq_hold_span(p,length,earlier,n->note)<=back))continue;
   /* Runtime caps gate duration to pattern length. */
   out->hold_bits[n->note>>3]|=(uint8_t)(1u<<(n->note&7));
   ++out->hold_voices;
  }
 }
 for(uint32_t k=0;k<16;k++){
  uint8_t bits=out->hold_bits[k],overlap=(uint8_t)(bits&out->trigger_bits[k]);
  while(bits){out->hold_pitches++;bits&=(uint8_t)(bits-1);}
  while(overlap){out->overlap_pitches++;overlap&=(uint8_t)(overlap-1);}
 }
}

uint8_t etch_seq_hold_toggle(struct etch_native_pattern *p,uint16_t step,uint8_t note){
 if(!p||step>=ETCH_SEQ_STEPS||note>127)return 0;
 struct etch_seq_step *s=&p->steps[step];
 for(uint8_t i=0;i<s->hold_count;i++)if(s->holds[i]==note){
  for(uint8_t j=i;j+1<s->hold_count;j++){s->holds[j]=s->holds[j+1];s->hold_ago[j]=s->hold_ago[j+1];}
  s->holds[s->hold_count-1]=0;s->hold_ago[s->hold_count-1]=0;--s->hold_count;return 2;
 }
 if(s->hold_count>=ETCH_SEQ_NOTES)return 0;
 s->holds[s->hold_count]=note;s->hold_ago[s->hold_count]=0;++s->hold_count;return 1;
}
uint16_t etch_seq_hold_span(const struct etch_native_pattern *p,uint16_t steps,uint16_t trigger,uint8_t note){
 if(!p||!steps||steps>ETCH_SEQ_STEPS||trigger>=steps||note>127)return 1;
 /* A new TRIGGER placed on an incoming HOLD is a separate note instance.
  * It must not inherit the old instance's upcoming HOLD chain. */
 const struct etch_seq_step *start=&p->steps[trigger];
 for(uint8_t j=0;j<start->hold_count&&j<ETCH_SEQ_NOTES;j++)
  if(start->holds[j]==note&&!start->hold_ago[j])return 1;
 uint16_t span=1;
 for(uint16_t i=1;i<steps;i++){
  uint16_t index=(uint16_t)((trigger+i)%steps);
  const struct etch_seq_step *s=&p->steps[index];uint8_t found=0;
  for(uint8_t j=0;j<s->hold_count&&j<ETCH_SEQ_NOTES;j++)if(s->holds[j]==note){
   /* A referenced HOLD belongs only to its authored trigger instance.
    * Zero means V6/V7 pitch-level HOLD, retained for legacy projects. */
   uint8_t ago=s->hold_ago[j];
   if(!ago||((uint8_t)(index-ago)==(uint8_t)trigger))found=1;
  }
  if(!found)break;
  ++span;
 }
 return span;
}
uint16_t etch_seq_hold_owner(const struct etch_native_pattern *p,uint16_t step,uint8_t pitch){
 if(!p||step>=ETCH_SEQ_STEPS||pitch>127)return 0xffffu;
 const struct etch_seq_step *s=&p->steps[step];
 for(uint8_t i=0;i<s->hold_count&&i<ETCH_SEQ_NOTES;i++)if(s->holds[i]==pitch){
  return s->hold_ago[i]?(uint16_t)((step+256u-s->hold_ago[i])&255u):0xffffu;
 }
 return 0xffffu;
}
uint8_t etch_seq_hold_link(struct etch_native_pattern *p,uint16_t length,uint16_t step,uint8_t pitch,uint16_t origin){
 if(!p||!length||length>ETCH_SEQ_STEPS||step>=length||origin>=length||origin==step||pitch>127)return 0;
 uint16_t ago=(uint16_t)((step+256u-origin)&255u);
 if(!ago)return 0;
 const struct etch_seq_step *src=&p->steps[origin];uint8_t exists=0;
 for(uint8_t j=0;j<src->count&&j<ETCH_SEQ_NOTES;j++)if(src->notes[j].note==pitch&&src->notes[j].velocity){exists=1;break;}
 if(!exists)return 0;
 struct etch_seq_step *s=&p->steps[step];
 for(uint8_t i=0;i<s->hold_count&&i<ETCH_SEQ_NOTES;i++)if(s->holds[i]==pitch&&s->hold_ago[i]==(uint8_t)ago)return 2;
 /* V6/V7 pitch-level HOLD becomes a directed link when first edited. */
 for(uint8_t i=0;i<s->hold_count&&i<ETCH_SEQ_NOTES;i++)if(s->holds[i]==pitch&&!s->hold_ago[i]){
  s->hold_ago[i]=(uint8_t)ago;return 2;
 }
 if(s->hold_count>=ETCH_SEQ_NOTES)return 0;
 s->holds[s->hold_count]=pitch;s->hold_ago[s->hold_count]=(uint8_t)ago;++s->hold_count;return 1;
}
uint8_t etch_seq_hold_unlink(struct etch_native_pattern *p,uint16_t step,uint8_t pitch,uint16_t origin){
 if(!p||step>=ETCH_SEQ_STEPS||pitch>127||(origin!=0xffffu&&(origin>=ETCH_SEQ_STEPS||origin==step)))return 0;
 uint8_t ago=origin==0xffffu?0:(uint8_t)(step-origin);
 struct etch_seq_step *s=&p->steps[step];
 for(uint8_t i=0;i<s->hold_count&&i<ETCH_SEQ_NOTES;i++)if(s->holds[i]==pitch&&s->hold_ago[i]==ago){
  for(uint8_t j=i;j+1<s->hold_count;j++){s->holds[j]=s->holds[j+1];s->hold_ago[j]=s->hold_ago[j+1];}
  --s->hold_count;s->holds[s->hold_count]=0;s->hold_ago[s->hold_count]=0;return 1;
 }
 return 0;
}
uint16_t etch_seq_hold_source_cycle(const struct etch_native_pattern *p,uint16_t length,uint16_t step,uint8_t pitch,uint16_t after,int8_t direction){
 if(!p||length<2||length>ETCH_SEQ_STEPS||step>=length||pitch>127)return 0xffffu;
 /* Circularly scan possible owner distances, never the current step. */
 uint16_t start=0;
 if(after<length&&after!=step)start=(uint16_t)((step+length-after)%length);
 for(uint16_t trial=1;trial<length;trial++){
  uint16_t dist;
  if(!start)dist=direction<0?(uint16_t)(length-trial):trial;
  else dist=(uint16_t)(((uint32_t)(start-1)+(direction<0?(length-1u)-(trial-1u):trial))%(length-1u)+1u);
  uint16_t source=(uint16_t)((step+length-dist)%length);
  const struct etch_seq_step *s=&p->steps[source];
  for(uint8_t i=0;i<s->count&&i<ETCH_SEQ_NOTES;i++)if(s->notes[i].note==pitch&&s->notes[i].velocity)return source;
 }
 return 0xffffu;
}
uint8_t etch_seq_bake_chord(struct etch_native_pattern *p,uint16_t step,uint8_t note,const struct etch_harmony_settings *h,uint8_t velocity,uint8_t length){
 if(!p||step>=ETCH_SEQ_STEPS||!velocity||!length||note>127)return 0;
 uint8_t chord[4];uint8_t count=etch_chord_notes(note,h,chord);if(!count)return 0;
 struct etch_seq_step trial=p->steps[step];
 for(uint8_t i=0;i<count;i++){
  uint8_t index=0;while(index<trial.count&&trial.notes[index].note!=chord[i])index++;
  if(index<trial.count){trial.notes[index].velocity=velocity;trial.notes[index].length=length;}
  else{if(trial.count>=ETCH_SEQ_NOTES)return 0;
   trial.notes[trial.count].note=chord[i];trial.notes[trial.count].velocity=velocity;
   trial.notes[trial.count++].length=length;
  }
 }
 p->steps[step]=trial;return count;
}

/* Follow desktop harmony's BAKE semantics: chord derives from lowest note,
 * LOCK quantizes/deduplicates when chord is OFF, GUIDE/OFF leaves notes alone.
 * Holds and sound parameter locks are *never* modified by BAKE. */
static int bake_transform(const struct etch_seq_step *src,
 const struct etch_harmony_settings *h,struct etch_seq_step *dst){
 *dst=*src;
 if(!src->count)return 0;
 if(src->count>ETCH_SEQ_NOTES)return -1;
 if(h->chord){
  struct etch_seq_note lowest=src->notes[0];
  for(uint8_t k=1;k<src->count;k++)if(src->notes[k].note<lowest.note)lowest=src->notes[k];
  uint8_t notes[4];uint8_t n=etch_chord_notes(lowest.note,h,notes);
  if(!n||n>ETCH_SEQ_NOTES)return -1;
  dst->count=n;
  for(uint8_t k=0;k<n;k++){
   dst->notes[k].note=notes[k];
   uint16_t v=(uint16_t)lowest.velocity*(100u-4u*k)/100u;
   dst->notes[k].velocity=v?(uint8_t)v:1;
   dst->notes[k].length=lowest.length;
  }
 }else if(h->keylock==2){
  dst->count=0;
  for(uint8_t k=0;k<src->count;k++){
   struct etch_seq_note n=src->notes[k];
   if(n.note>127||!n.length||!n.velocity)return -1;
   n.note=etch_quantize_note(n.note,h);
   uint8_t j=0;
   for(;j<dst->count;j++)if(dst->notes[j].note==n.note)break;
   if(j<dst->count){
    if(n.velocity>dst->notes[j].velocity)dst->notes[j].velocity=n.velocity;
    if(n.length>dst->notes[j].length)dst->notes[j].length=n.length;
   }else if(dst->count<ETCH_SEQ_NOTES)dst->notes[dst->count++]=n;
   else return -1;
  }
  for(uint8_t i=0;i<dst->count;i++)for(uint8_t j=i+1;j<dst->count;j++)
   if(dst->notes[j].note<dst->notes[i].note){struct etch_seq_note swap=dst->notes[i];dst->notes[i]=dst->notes[j];dst->notes[j]=swap;}
 }
 if(src->count!=dst->count)return 1;
 for(uint8_t i=0;i<dst->count;i++)
  if(src->notes[i].note!=dst->notes[i].note ||
     src->notes[i].velocity!=dst->notes[i].velocity ||
     src->notes[i].length!=dst->notes[i].length)return 1;
 return 0;
}
int etch_seq_bake_harmony(struct etch_native_pattern *p,uint16_t length,
 uint16_t selected,uint8_t scope,const struct etch_harmony_settings *h,uint8_t commit){
 if(!p||!h||!length||length>ETCH_SEQ_STEPS||selected>=length||scope>1||
    h->root>11||h->mode>8||h->keylock>2||h->chord>2)return -1;
 uint16_t begin=scope?0:selected,end=scope?length:(uint16_t)(selected+1);
 int changed=0;
 /* Validate all candidates before mutating a single step. */
 for(uint16_t i=begin;i<end;i++){
  struct etch_seq_step trial;
  int rc=bake_transform(&p->steps[i],h,&trial);
  if(rc<0)return -1;
  if(rc)for(uint8_t k=0;k<p->steps[i].count;k++){
   uint8_t pitch=p->steps[i].notes[k].note,found=0;
   if(!has_linked_hold(p,i,pitch))continue;
   for(uint8_t j=0;j<trial.count;j++)if(trial.notes[j].note==pitch)found=1;
   if(!found)return -1; /* no orphaned cross-step HOLD source */
  }
  changed+=rc;
 }
 if(commit)for(uint16_t i=begin;i<end;i++){
  struct etch_seq_step trial;
  if(bake_transform(&p->steps[i],h,&trial)>0)p->steps[i]=trial;
 }
 return changed;
}

int etch_harmony_guide_build(const struct etch_harmony_settings *h,
 uint8_t note,struct etch_harmony_guide *out){
 if(!h||!out||h->root>11||h->mode>8||h->chord>2||h->keylock>2||note>127)return -1;
 out->scale_mask=out->chord_mask=0;
 for(uint8_t i=0;i<7;i++)out->scale_mask|=(uint16_t)(1u<<((h->root+intervals[h->mode][i])%12u));
 out->count=etch_chord_notes(note,h,out->notes);
 for(uint8_t i=0;i<out->count;i++)out->chord_mask|=(uint16_t)(1u<<(out->notes[i]%12u));
 return 0;
}
