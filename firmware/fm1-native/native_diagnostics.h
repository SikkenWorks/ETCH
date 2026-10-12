/* SPDX-License-Identifier: GPL-3.0-only
 * Read-only on-device panel/DSP diagnostics. No storage writes and no
 * allocations. CPU DSP excludes nested TIMER5 input/USB work. WALL includes it.
 * Both are on-target estimates until measured on physical FM1 hardware.
 */
#pragma once
#include <stdint.h>
#define ETCH_DIAG_ENCODERS 7u
struct etch_diag_panel {
 uint32_t turns[ETCH_DIAG_ENCODERS];
 uint32_t key_seen,button_seen,observations;
 /* A single direction is not enough to prove the encoder's full quadrature path. */
 uint8_t clockwise,anticlockwise;
};
#define ETCH_DIAG_KEYS_MASK 0x7ffffffu
#define ETCH_DIAG_BUTTONS_MASK 0x3fffu
#define ETCH_DIAG_ENCODERS_MASK 0x7fu
static inline uint32_t etch_diag_abs(int32_t x){return x<0?(uint32_t)(-(int64_t)x):(uint32_t)x;}
static inline uint32_t etch_diag_add(uint32_t a,uint32_t b){return b>UINT32_MAX-a?UINT32_MAX:a+b;}
static inline void etch_diag_panel_record(struct etch_diag_panel *d,const int32_t enc[ETCH_DIAG_ENCODERS],uint32_t keys,uint32_t buttons){
 if(!d||!enc)return;
 for(uint32_t i=0;i<ETCH_DIAG_ENCODERS;i++){
  d->turns[i]=etch_diag_add(d->turns[i],etch_diag_abs(enc[i]));
  if(enc[i]>0)d->clockwise|=(uint8_t)(1u<<i);
  if(enc[i]<0)d->anticlockwise|=(uint8_t)(1u<<i);
 }
 d->key_seen|=keys&ETCH_DIAG_KEYS_MASK;d->button_seen|=buttons&ETCH_DIAG_BUTTONS_MASK;
 d->observations=etch_diag_add(d->observations,1u);
}
static inline uint32_t etch_diag_count(uint32_t bits){uint32_t n=0;while(bits){bits&=bits-1u;n++;}return n;}
static inline uint32_t etch_diag_panel_missing_keys(const struct etch_diag_panel *d){
 return d?etch_diag_count(ETCH_DIAG_KEYS_MASK&~d->key_seen):27u;
}
static inline uint32_t etch_diag_panel_missing_buttons(const struct etch_diag_panel *d){
 return d?etch_diag_count(ETCH_DIAG_BUTTONS_MASK&~d->button_seen):14u;
}
static inline uint32_t etch_diag_panel_missing_encoder_directions(const struct etch_diag_panel *d){
 return d?etch_diag_count(ETCH_DIAG_ENCODERS_MASK&~d->clockwise)+etch_diag_count(ETCH_DIAG_ENCODERS_MASK&~d->anticlockwise):14u;
}
static inline uint8_t etch_diag_panel_complete(const struct etch_diag_panel *d){
 return d&&!etch_diag_panel_missing_keys(d)&&!etch_diag_panel_missing_buttons(d)&&!etch_diag_panel_missing_encoder_directions(d);
}
static inline uint32_t etch_diag_cpu_permille(uint32_t ticks,uint32_t deadline){
 if(!deadline)return 0;
 uint64_t p=((uint64_t)ticks*1000u+deadline/2u)/deadline;
 return p>9999?9999:(uint32_t)p;
}
static inline uint32_t etch_diag_abspeak(int32_t sample){return sample<0?(uint32_t)(-sample):(uint32_t)sample;}
