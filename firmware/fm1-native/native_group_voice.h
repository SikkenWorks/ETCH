/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH GROUPS: compact ownership table for the shared eight-voice allocator.
 * The table is bound by the chosen allocator slot, never by pitch alone.
 * No allocation, no I/O; caller performs allocation before binding.
 */
#ifndef ETCH_NATIVE_GROUP_VOICE_H
#define ETCH_NATIVE_GROUP_VOICE_H
#include <stdint.h>
#include "native_group_router.h"
#define ETCH_GROUP_VOICE_CAPACITY 8u
struct etch_group_voice_owner {
 uint16_t identity;
 uint32_t generation;
 uint8_t group;
 uint8_t active;
};
struct etch_group_voice_map {
 struct etch_group_voice_owner slot[ETCH_GROUP_VOICE_CAPACITY];
};
static inline void etch_group_voice_reset(struct etch_group_voice_map *m){
 if(!m)return;
 for(uint8_t i=0;i<ETCH_GROUP_VOICE_CAPACITY;i++){
  m->slot[i].identity=0;m->slot[i].generation=0;
  m->slot[i].group=0;m->slot[i].active=0;
 }
}
/* Call after audio allocator chooses slot. Generation is allocator birth serial,
 * preventing stale note-off from detaching the newly stolen/reused slot. */
static inline uint8_t etch_group_voice_bind(struct etch_group_voice_map *m,
 uint8_t slot,uint8_t group,uint16_t identity,uint32_t generation){
 if(!m||slot>=ETCH_GROUP_VOICE_CAPACITY||group>2u||!generation)return 0;
 m->slot[slot]=(struct etch_group_voice_owner){identity,generation,group,1};
 return 1;
}
static inline uint8_t etch_group_voice_owner(const struct etch_group_voice_map *m,
 uint8_t slot,uint32_t generation){
 if(!m||slot>=ETCH_GROUP_VOICE_CAPACITY)return 255u;
 const struct etch_group_voice_owner *v=&m->slot[slot];
 return v->active && v->generation==generation?v->group:255u;
}
static inline uint8_t etch_group_voice_release(struct etch_group_voice_map *m,
 uint8_t slot,uint16_t identity,uint32_t generation){
 if(!m||slot>=ETCH_GROUP_VOICE_CAPACITY)return 0;
 struct etch_group_voice_owner *v=&m->slot[slot];
 if(!v->active||v->generation!=generation||v->identity!=identity)return 0;
 v->active=0;return 1;
}
/* Note-on is consumed while recording a keyboard range; it must not sound. */
static inline uint8_t etch_group_should_suppress_note(const struct etch_group_capture *c){
 return c&&(c->phase==ETCH_GROUP_WAIT_START||c->phase==ETCH_GROUP_WAIT_END);
}
#endif
