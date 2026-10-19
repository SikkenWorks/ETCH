/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH keyboard GROUPS: bounded, flash-independent configuration and capture.
 * Routing is mutually exclusive: main patch 0, extra patches 1 and 2.
 * Audio voice binding and persisted PJT upgrade must be implemented separately.
 */
#ifndef ETCH_NATIVE_GROUP_ROUTER_H
#define ETCH_NATIVE_GROUP_ROUTER_H
#include <stdint.h>
struct etch_key_group {uint8_t enabled,preset,start,end;};
struct etch_key_groups {struct etch_key_group group[2];};
enum etch_group_capture_phase {ETCH_GROUP_IDLE=0,ETCH_GROUP_WAIT_START=1,ETCH_GROUP_WAIT_END=2};
struct etch_group_capture {uint8_t slot,phase,start;};
static inline void etch_key_groups_init(struct etch_key_groups *s){
 if(!s)return;
 for(unsigned i=0;i<2u;i++){
  s->group[i].enabled=0;s->group[i].preset=0;
  s->group[i].start=0;s->group[i].end=127;
 }
}
static inline uint8_t etch_key_groups_valid(const struct etch_key_groups *s){
 if(!s)return 0;
 for(unsigned i=0;i<2u;i++){
  const struct etch_key_group *g=&s->group[i];
  if(g->enabled>1u||g->preset>=32u||g->start>=g->end||g->end>127u)return 0;
 }
 if(s->group[0].enabled&&s->group[1].enabled&&
    s->group[0].start<=s->group[1].end&&s->group[1].start<=s->group[0].end)return 0;
 return 1;
}
static inline uint8_t etch_key_group_set(struct etch_key_groups *s,uint8_t slot,
 uint8_t preset,uint8_t start,uint8_t end,uint8_t enabled){
 if(!s||slot>=2u||preset>=32u||start>=end||end>127u||enabled>1u)return 0;
 struct etch_key_group old=s->group[slot];
 s->group[slot]=(struct etch_key_group){enabled,preset,start,end};
 if(!etch_key_groups_valid(s)){s->group[slot]=old;return 0;}
 return 1;
}
static inline uint8_t etch_key_group_route(const struct etch_key_groups *s,uint8_t note){
 if(!s||note>127u)return 0;
 for(uint8_t i=0;i<2u;i++)if(s->group[i].enabled&&note>=s->group[i].start&&note<=s->group[i].end)return (uint8_t)(i+1u);
 return 0;
}
/* The caller feeds note-on events from BOTH FM1 panel and external MIDI here.
 * Returns 0 waiting, 1 completed, 2 rejected, 3 no active capture.
 * Rejected END remains in WAIT_END, allowing another note immediately.
 */
static inline void etch_group_capture_begin(struct etch_group_capture *c,uint8_t slot){
 if(!c)return;
 c->slot=slot<2u?slot:0u;c->phase=slot<2u?ETCH_GROUP_WAIT_START:ETCH_GROUP_IDLE;c->start=0u;
}
static inline uint8_t etch_group_capture_note(struct etch_group_capture *c,
 struct etch_key_groups *groups,uint8_t note,uint8_t preset){
 if(!c||!groups||c->phase==ETCH_GROUP_IDLE)return 3u;
 if(note>127u)return 2u;
 if(c->phase==ETCH_GROUP_WAIT_START){c->start=note;c->phase=ETCH_GROUP_WAIT_END;return 0u;}
 if(note<=c->start||!etch_key_group_set(groups,c->slot,preset,c->start,note,1u))return 2u;
 c->phase=ETCH_GROUP_IDLE;return 1u;
}
static inline void etch_group_capture_cancel(struct etch_group_capture *c){if(c)c->phase=ETCH_GROUP_IDLE;}
/* Fixed 8-byte payload. Reserved bits rejected to avoid silent corruption.
 * This is not yet integrated into PJT28 flash serialization. */
static inline void etch_key_groups_pack(const struct etch_key_groups *s,uint8_t out[8]){
 if(!s||!out)return;
 for(unsigned i=0;i<2u;i++){
  out[4u*i+0u]=s->group[i].enabled;
  out[4u*i+1u]=s->group[i].preset;
  out[4u*i+2u]=s->group[i].start;
  out[4u*i+3u]=s->group[i].end;
 }
}
static inline uint8_t etch_key_groups_unpack(struct etch_key_groups *s,const uint8_t in[8]){
 if(!s||!in)return 0;
 struct etch_key_groups tmp;
 for(unsigned i=0;i<2u;i++){
  tmp.group[i]=(struct etch_key_group){in[4u*i],in[4u*i+1u],in[4u*i+2u],in[4u*i+3u]};
 }
 if(!etch_key_groups_valid(&tmp))return 0;
 *s=tmp;return 1;
}
#endif
