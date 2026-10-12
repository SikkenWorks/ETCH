/* SPDX-License-Identifier: GPL-3.0-only
 * Native model matching ETCH's 256-step, 8-note host performance format.
 * Pattern snapshots have a capacity-bounded V5 flash codec; map not hardware-verified.
 */
#pragma once
#include <stdint.h>
#define ETCH_SEQ_STEPS 256u
#define ETCH_SEQ_NOTES 8u
#define ETCH_SEQ_LOCKS 6u
#define ETCH_ARP_PROGRAM_STEPS 16u
/* A custom arpeggio cycles through authored held-key/octave indices.
 * 0..31 indexes sorted held keys x octaves; 0xff is a rest.
 * length=0 means no custom program (legacy default). */
struct etch_arp_program {uint8_t length,steps[ETCH_ARP_PROGRAM_STEPS];};
/* ARP mode 7: deterministic, patch-seeded 16-step riff in the selected scale.
 * The mode is independent of the existing user-edited ARP program. */
int etch_arp_program_valid(const struct etch_arp_program *p);
void etch_arp_program_init(struct etch_arp_program *p);
uint8_t etch_arp_program_pick(const struct etch_arp_program *p,uint32_t step,uint32_t held_total);
#define ETCH_LOCK_MORPH 0u
#define ETCH_LOCK_DEPTH 1u
#define ETCH_LOCK_CUTOFF 2u
#define ETCH_LOCK_CHORUS 3u
#define ETCH_LOCK_DELAY 4u
#define ETCH_LOCK_REVERB 5u
struct etch_seq_note {uint8_t note,velocity,length;};
struct etch_seq_step {
 uint8_t count,lock_mask,hold_count;
 uint8_t holds[ETCH_SEQ_NOTES]; /* independent authored HOLD lane, never an attack */
 uint8_t hold_ago[ETCH_SEQ_NOTES]; /* V8: 1..255 = trigger-step distance; 0 = legacy pitch HOLD */
 struct etch_seq_note notes[ETCH_SEQ_NOTES];
 uint16_t locks[ETCH_SEQ_LOCKS];
};
struct etch_native_pattern {struct etch_seq_step steps[ETCH_SEQ_STEPS];};
struct etch_harmony_settings {uint8_t root,mode,keylock,chord;};
uint8_t etch_riff_pick(uint32_t seed,uint8_t step,uint8_t base,const struct etch_harmony_settings *h,uint8_t octaves);
uint32_t etch_riff_regenerate(uint32_t previous);
/* Bits 7 + 0..5 of patch.flags persist ARP controls without expanding the
 * existing V6 patch record. Zero/old flags retain legacy defaults. */
uint8_t etch_arp_pack_flags(uint8_t rate,uint8_t octaves,uint8_t gate_index);
void etch_arp_unpack_flags(uint8_t flags,uint8_t *rate,uint8_t *octaves,uint8_t *gate_index);
/* HOLD means a note whose own TRIGGER occurred on an earlier step and whose
 * length reaches the queried step. TRIGGER means a new attack on this step.
 * A pitch can appear in both layers independently (retrigger over sustain).
 */
struct etch_step_layers {
 uint8_t trigger_count,hold_pitches,overlap_pitches;
 uint16_t hold_voices;
 uint8_t trigger_bits[16],hold_bits[16];
};
void etch_seq_layers(const struct etch_native_pattern *p,uint16_t length,uint16_t step,struct etch_step_layers *out);
/* Return 1 if added, 2 if removed, 0 for invalid/full; never removes a HOLD. */
uint8_t etch_seq_toggle(struct etch_native_pattern *p,uint16_t step,uint8_t note,uint8_t velocity,uint8_t length);
void etch_pattern_clear(struct etch_native_pattern *p);
uint8_t etch_seq_put(struct etch_native_pattern *p,uint16_t step,uint8_t note,uint8_t velocity,uint8_t length);
/* Atomic NOTES-page edit: 1 updated, 2 duplicate pitch rejected,
 * 3 linked HOLD must be removed first, 0 invalid.
 * A collision never mutates or removes the existing note or HOLD lanes. */
uint8_t etch_seq_note_edit(struct etch_native_pattern *p,uint16_t step,uint8_t index,
 uint8_t note,uint8_t velocity,uint8_t length);
/* Remove only a TRIGGER note, preserving independent HOLD and parameter locks.
 * Returns 1 removed, 2 blocked (referenced by HOLD), 0 invalid. */
uint8_t etch_seq_note_remove(struct etch_native_pattern *p,uint16_t step,uint8_t index);
/* GRID follows 8 columns x 2 rows per page: page delta=16, row delta=8. */
uint16_t etch_seq_grid_move(uint16_t current,uint16_t length,int32_t pages,
 int32_t columns,int32_t rows,int32_t select);
/* Explicit HOLD lane, distinct from per-note duration: returns 1 added, 2 removed, 0 invalid/full. */
uint8_t etch_seq_hold_toggle(struct etch_native_pattern *p,uint16_t step,uint8_t note);
uint16_t etch_seq_hold_span(const struct etch_native_pattern *p,uint16_t steps,uint16_t trigger,uint8_t note);
/* Explicitly attach a HOLD to the note TRIGGER on `origin` (different step).
 * Returns 1 added, 2 already present, 0 rejected without mutation. */
uint8_t etch_seq_hold_link(struct etch_native_pattern *p,uint16_t length,uint16_t step,uint8_t pitch,uint16_t origin);
/* Removes precisely the requested trigger instance. 1 removed, 0 absent/rejected.
 * origin=0xffffu targets a legacy unbound HOLD. */
uint8_t etch_seq_hold_unlink(struct etch_native_pattern *p,uint16_t step,uint8_t pitch,uint16_t origin);
/* Returns the origin step of the first matching pitch, or 0xffff for legacy/absent. */
uint16_t etch_seq_hold_owner(const struct etch_native_pattern *p,uint16_t step,uint8_t pitch);
/* Find the previous/next earlier same-pitch trigger using distance 1..length-1.
 * Return 0xffff when none exists. `after` is the current origin (or 0xffff). */
uint16_t etch_seq_hold_source_cycle(const struct etch_native_pattern *p,uint16_t length,uint16_t step,uint8_t pitch,uint16_t after,int8_t direction);
/* Bake chord to one step atomically, without discarding existing notes; returns 0 on lack of space. */
uint8_t etch_seq_bake_chord(struct etch_native_pattern *p,uint16_t step,uint8_t note,const struct etch_harmony_settings *h,uint8_t velocity,uint8_t length);
uint8_t etch_quantize_note(uint8_t note,const struct etch_harmony_settings *h);
uint8_t etch_chord_notes(uint8_t note,const struct etch_harmony_settings *h,uint8_t out[4]);

/* Non-destructive harmony preview and transactional pattern BAKE.  `scope` is
 * 0=one selected step, 1=all steps in the selected pattern.  Returns the
 * number of changed steps, or -1 on invalid input.  `commit=0` never writes. */
int etch_seq_bake_harmony(struct etch_native_pattern *p,uint16_t length,
 uint16_t selected,uint8_t scope,const struct etch_harmony_settings *h,
 uint8_t commit);

/* 12-bit pitch-class set: bit 0=C, bit 11=B. GUIDE returns both scale
 * membership and the chord currently previewed, with actual MIDI pitches. */
struct etch_harmony_guide {uint16_t scale_mask,chord_mask;uint8_t notes[4],count;};
int etch_harmony_guide_build(const struct etch_harmony_settings *h,
 uint8_t note,struct etch_harmony_guide *out);

/* User-facing ARP octave range -2,-1,+1..+4, encoded 6,5,1..4.
 * No project format upgrade: V6+ existing flags still decode identically. */
static inline uint8_t etch_arp_octave_turn(uint8_t current,int32_t delta){
 static const uint8_t choices[6]={6u,5u,1u,2u,3u,4u};
 uint8_t idx=2u;for(uint8_t i=0;i<6;i++)if(choices[i]==current){idx=i;break;}
 int32_t next=(int32_t)idx+delta;
 if(next<0)next=0;if(next>5)next=5;return choices[next];
}
static inline int8_t etch_arp_octave_display(uint8_t value){
 return value==6u?-2:value==5u?-1:(int8_t)value;
}
