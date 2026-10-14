/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH native audio effects. Fixed buffers, no allocation, deterministic work.
 * Effects are global, post voice-mix; buffers belong to a single instrument.
 */
#pragma once
#include <stdint.h>
#define ETCH_DELAY_SIZE 16384u
#define ETCH_CHORUS_SIZE 2048u
#define ETCH_REVERB_SIZE 4096u
/* Values are Q15 (0..32767); routing: 0 serial, 1 parallel. */
struct etch_fx_settings {
 uint16_t chorus_rate,chorus_depth,delay_time,delay_feedback,reverb_size,reverb_decay;
 uint8_t routing;
 /* V0.96 project-scoped Ring Mod + 3-band channel EQ; Q15 controls.
  * EQ center (16384) is unity, each band spans 0..2x. */
 uint16_t ring_mix,ring_rate,ring_shape,ring_bias;
 uint16_t eq_low,eq_mid,eq_high,eq_hpf;
 /* V1.02 global/preset effect parameters. */
 uint16_t comp_threshold,comp_ratio,comp_attack,comp_release;
 uint16_t trem_rate,trem_depth,trem_shape,trem_mix;
 uint16_t phaser_rate,phaser_depth,phaser_feedback,phaser_mix;
 uint16_t flanger_rate,flanger_depth,flanger_feedback,flanger_mix;
 uint16_t decim_bits,decim_rate,decim_drive,decim_mix;
 uint16_t folder_fold,folder_symmetry,folder_tone,folder_mix;
 /* V1.04 PJT15: global/preset tape instabilities */
 uint16_t tape_wow,tape_flutter,tape_age,tape_mix;
 /* V1.06 PJT16 additive FX, each block of four Q15 controls. */
 uint16_t reverse_time,reverse_feed,reverse_tone,reverse_mix;
 uint16_t vibrato_rate,vibrato_depth,vibrato_shape,vibrato_mix;
 uint16_t rotary_rate,rotary_depth,rotary_drive,rotary_mix;
 uint16_t wah_sens,wah_attack,wah_reso,wah_mix;
 /* PJT17: two lightweight pre-morph insert slots on each A/B waveform.
  * Types: 0=OFF,1=RING,2=DECIMATE,3=FOLD,4=TREMOLO,5=LOWPASS.
  * Each slot owns TYPE/DEPTH/MIX. No per-voice delay or reverb buffers.
  * These are separate from the global post-morph FX chain. */
 /* Each slot: TYPE 3-bit (0..5), DEPTH 7-bit, MIX 6-bit.
  * Fixed-point compaction saves 594B across the 32 preset snapshots.
  * Wave C FX V1.16: TARGET 5-bit (0..31) + DEPTH 11-bit.
  * PJT17..19 values are migrated in the project decoder. */
 uint16_t pre[2][2];
 uint16_t c_fx;
};
struct etch_native_fx {
 int16_t delay[ETCH_DELAY_SIZE],chorus[ETCH_CHORUS_SIZE],reverb[ETCH_REVERB_SIZE];
 uint32_t delay_pos,chorus_pos,reverb_pos,chorus_phase;
 struct etch_fx_settings applied;
 /* Bounded values: delay < 16384, chorus rate <= 10, tank taps < 4096.
  * Compact control state preserves the already-full fixed audio POOL. */
 uint16_t delay_samples,rate_step,tap1,tap2,tap3;
 uint8_t applied_valid;
 uint32_t ring_phase,ring_increment;
 int32_t ring_mix_smooth,eq_gain_smooth[3];
 int32_t eq_lp_low,eq_lp_high,eq_lp_hpf;
 uint16_t eq_hpf_alpha;
 uint32_t trem_phase,phaser_phase,flanger_phase,trem_step,phaser_step,flanger_step;
 int32_t trem_mix_smooth,phaser_mix_smooth,flanger_mix_smooth,decim_mix_smooth,folder_mix_smooth;
 int32_t comp_env,comp_gain,comp_target;
 uint8_t comp_tick;
 int32_t phaser_z[4],phaser_feedback_state,flanger_feedback_state;
 int16_t flanger_line[1024];uint16_t flanger_pos;
 int32_t decim_held,folder_tone_state,folder_baseline;
 uint16_t folder_last_fold,folder_last_symmetry;uint8_t folder_baseline_valid;uint16_t decim_counter;
 /* 512-sample dedicated tape delay; bounded 1 KiB. No dynamic allocation. */
 uint16_t tape_pos;
 uint32_t tape_wow_phase,tape_flutter_phase,tape_wow_step,tape_flutter_step;
 int32_t tape_mix_smooth,tape_tone_state;
 /* Reverse overlap: two alternating capture blocks; max block 8192 samples.
  * Capturing the next block never overwrites the reverse-playback block. */
 uint16_t reverse_pos,reverse_len;
 uint8_t reverse_bank,reverse_ready,reverse_subphase;int32_t reverse_tone_state,reverse_mix_smooth,reverse_accum;
 uint16_t vibrato_pos;
 uint32_t vibrato_phase,vibrato_step;int32_t vibrato_mix_smooth;
 uint32_t rotary_phase,rotary_step;int32_t rotary_mix_smooth,rotary_horn;
 int32_t wah_env,wah_lp,wah_hp,wah_mix_smooth;
 /* Incremental clearing of old time-based FX buffers in exact dry bypass.
  * Never memset tens of KiB in the realtime ISR. No preset/settings changes. */
 uint32_t dry_flush_cursor;
 uint8_t dry_was_wet,dry_flush_active;
 /* SPACE 2.0: 128 B shared diffusion line in .noinit and only 8 B of
  * control state. On-flash etch_fx_settings/PJT21 layout stays identical. */
 int32_t space_damp_state;
 uint16_t chorus_span,space_damp_alpha;
 int32_t reverb_mix_smooth;
};
static inline void etch_fx_defaults(struct etch_fx_settings *s){
 if(!s)return;
 s->chorus_rate=4096;s->chorus_depth=16384;
 s->delay_time=16384;s->delay_feedback=12288;
 s->reverb_size=32767;s->reverb_decay=20480;s->routing=0;
 s->ring_mix=0;s->ring_rate=8192;s->ring_shape=0;s->ring_bias=0;
 s->eq_low=16384;s->eq_mid=16384;s->eq_high=16384;s->eq_hpf=0;
 s->comp_threshold=16384;s->comp_ratio=0;s->comp_attack=8192;s->comp_release=16384;
 s->trem_rate=8192;s->trem_depth=16384;s->trem_shape=0;s->trem_mix=0;
 s->phaser_rate=8192;s->phaser_depth=16384;s->phaser_feedback=8192;s->phaser_mix=0;
 s->flanger_rate=8192;s->flanger_depth=16384;s->flanger_feedback=8192;s->flanger_mix=0;
 s->decim_bits=32767;s->decim_rate=0;s->decim_drive=0;s->decim_mix=0;
 s->folder_fold=0;s->folder_symmetry=16384;s->folder_tone=32767;s->folder_mix=0;
 s->tape_wow=8192;s->tape_flutter=8192;s->tape_age=8192;s->tape_mix=0;
 s->reverse_time=16384;s->reverse_feed=0;s->reverse_tone=24576;s->reverse_mix=0;
 s->vibrato_rate=8192;s->vibrato_depth=8192;s->vibrato_shape=0;s->vibrato_mix=0;
 s->rotary_rate=8192;s->rotary_depth=20000;s->rotary_drive=0;s->rotary_mix=0;
 s->wah_sens=16384;s->wah_attack=16384;s->wah_reso=8192;s->wah_mix=0;
 for(uint32_t w=0;w<2u;w++)for(uint32_t slot=0;slot<2u;slot++){
  s->pre[w][slot]=((uint16_t)64u<<3);
 }
 s->c_fx=0u;
}
static inline uint8_t etch_pre_type(const struct etch_fx_settings *s,uint8_t w,uint8_t j){return (uint8_t)(s->pre[w][j]&7u);}
static inline uint16_t etch_pre_depth(const struct etch_fx_settings *s,uint8_t w,uint8_t j){return (uint16_t)(((s->pre[w][j]>>3)&127u)*32767u/127u);}
static inline uint16_t etch_pre_mix(const struct etch_fx_settings *s,uint8_t w,uint8_t j){return (uint16_t)(((s->pre[w][j]>>10)&63u)*32767u/63u);}
static inline void etch_pre_set_type(struct etch_fx_settings *s,uint8_t w,uint8_t j,uint8_t type){s->pre[w][j]=(s->pre[w][j]&~7u)|(type>5u?5u:type);}
static inline void etch_pre_set_depth(struct etch_fx_settings *s,uint8_t w,uint8_t j,uint16_t depth){uint16_t v=(uint16_t)(((uint32_t)(depth>32767u?32767u:depth)*127u+16383u)/32767u);s->pre[w][j]=(s->pre[w][j]&~(127u<<3))|(v<<3);}
static inline void etch_pre_set_mix(struct etch_fx_settings *s,uint8_t w,uint8_t j,uint16_t mix){uint16_t v=(uint16_t)(((uint32_t)(mix>32767u?32767u:mix)*63u+16383u)/32767u);s->pre[w][j]=(s->pre[w][j]&~(63u<<10))|(v<<10);}
/* V1.16 packed C-FX control: 31 destinations, 0 is bypass. */
#define ETCH_C_FX_MAX_TARGET 31u
static inline uint8_t etch_c_fx_target(const struct etch_fx_settings *s){return (uint8_t)(s->c_fx&31u);}
static inline uint16_t etch_c_fx_depth(const struct etch_fx_settings *s){return (uint16_t)(((uint32_t)(s->c_fx>>5)*32767u)/2047u);}
static inline void etch_c_fx_set_target(struct etch_fx_settings *s,uint8_t target){s->c_fx=(uint16_t)((s->c_fx&~31u)|(target>31u?31u:target));}
static inline void etch_c_fx_set_depth(struct etch_fx_settings *s,uint16_t depth){s->c_fx=(uint16_t)((s->c_fx&31u)|((uint16_t)(((uint32_t)(depth>32767u?32767u:depth)*2047u+16383u)/32767u)<<5));}
/* Migration keeps all previously saved target IDs (0..6) and their depth. */
static inline uint16_t etch_c_fx_migrate_old(uint16_t legacy){
 uint16_t target=legacy&7u,depth=(uint16_t)(((uint32_t)(legacy>>3)*32767u)/8191u);
 if(target>6u)target=0u;
 return (uint16_t)(target|((uint32_t)depth*2047u/32767u<<5));
}
static inline uint16_t etch_c_fx_as_old(uint16_t modern){
 uint16_t target=modern&31u,depth=(uint16_t)(((uint32_t)(modern>>5)*32767u)/2047u);
 if(target>6u)target=0u;
 return (uint16_t)((target&7u)|(((uint32_t)depth*8191u/32767u)<<3));
}
/* All C-wave destinations have compact names suitable for 240px. */
static inline const char *etch_c_fx_name(uint8_t target){
 static const char *const names[32]={"OFF","PHASER RATE","DECIM BITS","DELAY TIME","FLANGE RATE","TREM DEPTH","EQ MID",
 "REVERB SIZE","REVERB DECAY","CHORUS RATE","CHORUS DEPTH","DELAY FEED","RING RATE","EQ LOW","EQ HIGH",
 "COMP THRESH","COMP RATIO","TREM RATE","PHASER DEPTH","FLANGE DEPTH","DECIM RATE","FOLDER FOLD",
 "TAPE WOW","TAPE FLUTTER","REVERSE TIME","VIBRATO RATE","ROTARY RATE","WAH SENS",
 "REVERB MIX","DELAY MIX","CHORUS MIX","EQ HIGHPASS"};
 return names[target&31u];
}

/* Controls-only test: do not infer FX activity from nonzero rates or
 * feedback values. Only MIX, compressor ratio, EQ gain/HPF and master sends
 * make the audio wet. Stored preset parameters remain completely untouched. */
static inline uint8_t etch_fx_controls_dry(const struct etch_fx_settings *c,
                                           uint16_t chorus,uint16_t delay,uint16_t reverb){
 if(!c||chorus||delay||reverb)return 0u;
 if(c->ring_mix||c->comp_ratio||c->trem_mix||c->phaser_mix||
    c->flanger_mix||c->decim_mix||c->folder_mix||c->tape_mix||
    c->reverse_mix||c->vibrato_mix||c->rotary_mix||c->wah_mix||c->eq_hpf)return 0u;
 /* Some very old zero-initialised settings had an implicit neutral EQ. */
 if(!c->eq_low&&!c->eq_mid&&!c->eq_high &&
    !c->ring_rate&&!c->ring_shape&&!c->ring_bias)return 1u;
 return c->eq_low==16384u&&c->eq_mid==16384u&&c->eq_high==16384u;
}
void etch_fx_reset(struct etch_native_fx *s);
int16_t etch_fx_process_ex(struct etch_native_fx *s,int16_t in,uint16_t chorus,uint16_t delay,uint16_t reverb,const struct etch_fx_settings *settings);
int16_t etch_fx_process(struct etch_native_fx *s,int16_t in,uint16_t chorus,uint16_t delay,uint16_t reverb);

/* Stereo return uses one shared delay/chorus/reverb buffer, with distinct
 * left/right wet taps. No second RAM-heavy effect instance. */
void etch_fx_process_stereo_ex(struct etch_native_fx *s,int16_t in,uint16_t chorus,uint16_t delay,uint16_t reverb,const struct etch_fx_settings *settings,int16_t *left,int16_t *right);
