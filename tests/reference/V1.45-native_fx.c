/* SPDX-License-Identifier: GPL-3.0-only
 * Bounded, allocation-free global FX. Settable LFO speed/depth, variable echo
 * time/feedback, reverb size/decay and serial or parallel signal routing.
 * The hard limiter and signed intermediate arithmetic keep feedback stable.
 */
#include "native_fx.h"
/* The FM1 application has a single global FX instance. These buffers occupy
 * verified spare .noinit SRAM rather than the saturated DMA/voice POOL.
 * They are explicitly cleared by etch_fx_reset before sound starts. */
static int16_t reverse_line[3584] __attribute__((section(".noinit")));
static int16_t vibrato_line[512] __attribute__((section(".noinit")));
static int16_t tape_line[512] __attribute__((section(".noinit")));
/* V1.22 SPACE 2.0: one 64-sample diffusion line shared by both channels.
 * Global rather than per-voice: fixed 128 B SRAM, zero heap allocation. */
static int16_t space_diffuser[64] __attribute__((section(".noinit")));
static int16_t clip16(int32_t v){return (int16_t)(v<-32768?-32768:v>32767?32767:v);}
static uint16_t qclamp(uint16_t x){return x>32767?32767:x;}
/* RC candidate F03: wet-output soft knee before the hardware DAC/USB stage.
 * No effect sends means exact bit-for-bit dry bypass. The knee starts at
 * 22,000 and asymptotically approaches 30,192 in Q15, avoiding full-scale
 * feedback spikes without resetting time-based FX state. This is deliberately
 * a cheap bounded integer operation, not loudness matching or DAC calibration.
 */
static int16_t etch_fx_wet_safety(int32_t x,uint8_t wet_active){
 if(!wet_active)return clip16(x);
 int32_t mag=x<0?-x:x;
 if(mag<=22000)return (int16_t)x;
 int32_t excess=mag-22000;
 int32_t limited=22000+(excess*8192)/(excess+8192);
 return (int16_t)(x<0?-limited:limited);
}
void etch_fx_reset(struct etch_native_fx *s){
 uint8_t *p=(uint8_t*)s;for(uint32_t i=0;i<sizeof(*s);i++)p[i]=0;
 for(uint32_t i=0;i<3584;i++)reverse_line[i]=0;
 for(uint32_t i=0;i<512;i++)vibrato_line[i]=0;
 for(uint32_t i=0;i<512;i++)tape_line[i]=0;
 for(uint32_t i=0;i<64;i++)space_diffuser[i]=0;
 s->eq_gain_smooth[0]=s->eq_gain_smooth[1]=s->eq_gain_smooth[2]=16384;
 s->comp_gain=32767;s->comp_target=32767;
}
/* Ring Mod and channel EQ run once after the voice sum, never per voice.
 * No heap, no trigonometry, no flash access inside the audio ISR. */
/* Fractional mono tap shared by the two chorus output phases. This avoids
 * 1-sample read-address jumps on slow chorus modulation (Q8 position). */
static int32_t etch_fx_chorus_read(const struct etch_native_fx *s,uint32_t offset_q8){
 uint32_t idx=(s->chorus_pos-(offset_q8>>8))&(ETCH_CHORUS_SIZE-1u);
 int32_t a=s->chorus[idx],b=s->chorus[(idx-1u)&(ETCH_CHORUS_SIZE-1u)];
 return a+(((b-a)*(int32_t)(offset_q8&255u))/256);
}
static int32_t etch_fx_slew(int32_t current,int32_t target){
 int32_t delta=target-current;
 if(!delta)return current;
 return current+(delta>0?(delta+63)/64:-((-delta+63)/64));
}
static int16_t etch_fx_ring_eq(struct etch_native_fx *s,int16_t sample,const struct etch_fx_settings *c){
 int32_t x=sample;
 /* Slew gain and wet controls over approximately 1.5ms @44.1kHz. */
 s->ring_mix_smooth=etch_fx_slew(s->ring_mix_smooth,qclamp(c->ring_mix));
 /* Many pre-V0.96 callers supply zeroed optional FX settings. Treat this
  * *legacy uninitialised* signature as neutral EQ, not as mute-all. */
 uint8_t legacy_zero=(uint8_t)(!c->ring_rate&&!c->ring_mix&&!c->ring_shape&&!c->ring_bias&&
     !c->eq_low&&!c->eq_mid&&!c->eq_high&&!c->eq_hpf);
 for(uint32_t i=0;i<3u;i++){
  uint16_t goal=legacy_zero?16384:(i==0?c->eq_low:i==1?c->eq_mid:c->eq_high);
  s->eq_gain_smooth[i]=etch_fx_slew(s->eq_gain_smooth[i],qclamp(goal));
 }
 if(s->ring_mix_smooth){
  uint32_t p=s->ring_phase>>16;
  int32_t triangle=p<32768u?(int32_t)(p*2u)-32767:98303-(int32_t)(p*2u);
  int32_t square=p<32768u?32767:-32767;
  int32_t shape=qclamp(c->ring_shape);
  int32_t carrier=(triangle*(32767-shape)+square*shape)/32767;
  int32_t bias=qclamp(c->ring_bias);
  carrier=(carrier*(32767-bias)+32767*bias)/32767;
  int32_t wet=(x*carrier)/32767;
  x+=(int32_t)(((int64_t)(wet-x)*s->ring_mix_smooth)/32767);
 }
 s->ring_phase+=s->ring_increment;
 /* One-pole crossovers: low <~200Hz, mid ~200Hz..~6kHz, high >~6kHz.
  * The three bands recombine exactly to dry when each gain is at unity. */
 s->eq_lp_low+=(int32_t)(((int64_t)(x-s->eq_lp_low)*900)/32768);
 s->eq_lp_high+=(int32_t)(((int64_t)(x-s->eq_lp_high)*9000)/32768);
 if(c->eq_hpf){
  s->eq_lp_hpf+=(int32_t)(((int64_t)(x-s->eq_lp_hpf)*s->eq_hpf_alpha)/32768);
  x-=s->eq_lp_hpf;
 }else s->eq_lp_hpf=x;
 if(s->eq_gain_smooth[0]!=16384||s->eq_gain_smooth[1]!=16384||s->eq_gain_smooth[2]!=16384){
  int32_t low=s->eq_lp_low,mid=s->eq_lp_high-low,high=x-s->eq_lp_high;
  x=(int32_t)(((int64_t)low*s->eq_gain_smooth[0]+(int64_t)mid*s->eq_gain_smooth[1]+
      (int64_t)high*s->eq_gain_smooth[2])/16384);
 }
 return clip16(x);
}

/* V1.02 six global FX: signed fixed point, no malloc/no libm. 44.1 kHz.
 * Mix parameters are slewed each sample. No DSP here touches flash/UI.
 * Every effect is exact bypass at its factory settings. */
/* A symmetric wavefolder must map digital silence to exact zero even
 * when the SYMMETRY knob is offset. Cache the zero-input transfer value
 * instead of doing a second eight-reflection calculation per audio sample. */
static int16_t fx_fold_transfer(int32_t x,uint16_t fold_control,uint16_t symmetry_control){
 int32_t symmetry=((int32_t)qclamp(symmetry_control)-16384)/2;
 int32_t fold=1+(int32_t)(qclamp(fold_control)*7u/32767u);
 int32_t y=(x+symmetry)*fold;
 for(int j=0;j<8;j++){
  if(y>32767)y=65534-y;
  else if(y<-32767)y=-65534-y;
  else break;
 }
 return clip16(y-symmetry);
}
static uint32_t fx_triangle(uint32_t phase){
 uint32_t p=phase>>16;return p<32768u?p*2u:131070u-p*2u;
}
static int16_t fx_v102_process(struct etch_native_fx *s,int16_t sample,const struct etch_fx_settings *c){
 int32_t x=sample,wet=0;
 uint16_t mix;
 /* Compressor: envelope follower, 16-sample control rate, smoothly applied
  * gain. Ratio 1:1 (zero control) provides exact bypass. */
 if(c->comp_ratio){
  int32_t mag=x<0?-x:x;
  int32_t atk=2+(32767u-qclamp(c->comp_attack))/512u;
  int32_t rel=1+(32767u-qclamp(c->comp_release))/2048u;
  int32_t step=mag>s->comp_env?atk:rel;
  s->comp_env+=(int32_t)(((int64_t)(mag-s->comp_env)*step)/32768);
  if(++s->comp_tick>=16){
   s->comp_tick=0;
   int32_t thr=2000+(32767u-qclamp(c->comp_threshold))*25000u/32767u;
   int32_t ratio=1+qclamp(c->comp_ratio)*11u/32767u;
   int32_t target=32767;
   if(s->comp_env>thr){
    int32_t limited=thr+(s->comp_env-thr)/ratio;
    target=(int32_t)(((int64_t)limited*32767)/s->comp_env);
   }
   s->comp_target=target;
  }
  s->comp_gain=etch_fx_slew(s->comp_gain,s->comp_target);
  x=(int32_t)(((int64_t)x*s->comp_gain)/32767);
 }else{s->comp_gain=32767;s->comp_target=32767;s->comp_env=0;s->comp_tick=0;}
 /* Tremolo: 0.15..15Hz, triangle -> square, wet/dry blend. */
 s->trem_mix_smooth=etch_fx_slew(s->trem_mix_smooth,qclamp(c->trem_mix));
 if(s->trem_mix_smooth){
  uint32_t tri=fx_triangle(s->trem_phase), square=(s->trem_phase&0x80000000u)?0u:65535u;
  uint32_t sh=qclamp(c->trem_shape);uint32_t lfo=(tri*(32767u-sh)+square*sh)/32767u;
  uint32_t depth=qclamp(c->trem_depth);
  uint32_t amp=32767u-(uint32_t)(((uint64_t)(65535u-lfo)*depth)/65535u);
  wet=(int32_t)(((int64_t)x*amp)/32767);
  x+=(int32_t)(((int64_t)(wet-x)*s->trem_mix_smooth)/32767);
 }
 s->trem_phase+=s->trem_step;
 /* V1.06 Vibrato: fractional variable delay, not amplitude tremolo.
  * Slow triangle/square morph varies read time by up to +/- ~3ms.
  * Read head always trails write head; no dynamic allocations or libm. */
 s->vibrato_mix_smooth=etch_fx_slew(s->vibrato_mix_smooth,qclamp(c->vibrato_mix));
 vibrato_line[s->vibrato_pos]=clip16(x);
 if(s->vibrato_mix_smooth){
  uint32_t tri=fx_triangle(s->vibrato_phase);
  uint32_t squ=(s->vibrato_phase&0x80000000u)?0u:65535u;
  uint32_t shape=qclamp(c->vibrato_shape);
  uint32_t lfo=(tri*(32767u-shape)+squ*shape)/32767u;
  int32_t offset=(int32_t)lfo-32767;
  int32_t shift=(int32_t)(((int64_t)offset*qclamp(c->vibrato_depth)*140u*256)/(32767*32767));
  uint32_t distance=(uint32_t)(200*256+shift);
  uint16_t a=(uint16_t)((s->vibrato_pos-(distance>>8))&511u);
  int32_t wetv=(vibrato_line[a]*(int32_t)(256-(distance&255u))+
     vibrato_line[(a-1u)&511u]*(int32_t)(distance&255u))/256;
  x+=(int32_t)(((int64_t)(wetv-x)*s->vibrato_mix_smooth)/32767);
 }
 s->vibrato_pos=(uint16_t)((s->vibrato_pos+1u)&511u);
 s->vibrato_phase+=s->vibrato_step;
 /* V1.06 Auto-Wah: smoothed envelope controls resonant-like bandpass.
  * Strictly bounded one-pole sections and wet slew, no unstable feedback. */
 s->wah_mix_smooth=etch_fx_slew(s->wah_mix_smooth,qclamp(c->wah_mix));
 if(s->wah_mix_smooth){
  int32_t mag=x<0?-x:x;
  uint32_t attack=4u+(32767u-qclamp(c->wah_attack))*1000u/32767u;
  uint32_t rel=16u;
  s->wah_env+=(int32_t)(((int64_t)(mag-s->wah_env)*(mag>s->wah_env?attack:rel))/32768);
  int32_t response=(int32_t)(((int64_t)s->wah_env*qclamp(c->wah_sens))/32767);
  if(response>24000)response=24000;
  uint32_t alpha=300u+(uint32_t)response*15000u/24000u;
  s->wah_lp+=(int32_t)(((int64_t)(x-s->wah_lp)*alpha)/32768);
  s->wah_hp+=(int32_t)(((int64_t)(s->wah_lp-s->wah_hp)*((alpha>>2)+100u))/32768);
  int32_t band=s->wah_lp-s->wah_hp;
  int32_t resonance=16384+(int32_t)qclamp(c->wah_reso)*16383/32767;
  int32_t wetwah=clip16((int32_t)(((int64_t)band*resonance)/16384));
  x+=(int32_t)(((int64_t)(wetwah-x)*s->wah_mix_smooth)/32767);
 }else s->wah_env=0;
 /* Four cascaded stable first-order all-pass stages with bounded feedback. */
 s->phaser_mix_smooth=etch_fx_slew(s->phaser_mix_smooth,qclamp(c->phaser_mix));
 if(s->phaser_mix_smooth){
  uint32_t tri=fx_triangle(s->phaser_phase);
  int32_t coeff=5000+(int32_t)(((uint64_t)tri*qclamp(c->phaser_depth)*22500u)/(65535u*32767u));
  int32_t fb=(int32_t)(((int64_t)s->phaser_feedback_state*qclamp(c->phaser_feedback)*6)/(32767u*10u));
  int32_t y=clip16(x+fb);
  for(int j=0;j<4;j++){
   int32_t v=s->phaser_z[j]-(int32_t)(((int64_t)coeff*y)/32768);
   s->phaser_z[j]=clip16(y+(int32_t)(((int64_t)coeff*v)/32768));
   y=clip16(v);
  }
  s->phaser_feedback_state=y;
  x+=(int32_t)(((int64_t)(y-x)*s->phaser_mix_smooth)/32767);
 }
 s->phaser_phase+=s->phaser_step;
 /* Flanger: 1024 signed samples (2 KiB), modulated 1..12ms delay. */
 s->flanger_mix_smooth=etch_fx_slew(s->flanger_mix_smooth,qclamp(c->flanger_mix));
 if(s->flanger_mix_smooth){
  uint32_t tri=fx_triangle(s->flanger_phase);
  uint32_t offset=32u+(uint32_t)(((uint64_t)tri*qclamp(c->flanger_depth)*490u)/(65535u*32767u));
  uint32_t index=(s->flanger_pos-offset)&511u;
  wet=s->flanger_line[index];
  int32_t fb=(int32_t)(((int64_t)wet*qclamp(c->flanger_feedback)*65)/(32767u*100u));
  s->flanger_line[s->flanger_pos]=clip16(x+fb);
  s->flanger_pos=(s->flanger_pos+1u)&511u;
  x+=(int32_t)(((int64_t)(wet-x)*s->flanger_mix_smooth)/32767);
 }
 s->flanger_phase+=s->flanger_step;
 /* Tape Instability: variable interpolated short delay yields real pitch wow
  * and flutter, not tremolo. The tape head is global once after voice mix.
  * Read distance: 180 +/- 90 (wow) +/- 15 (flutter) samples, always
  * >=75 samples, never reads the write head. Crossfade wet/dry smoothly.
  * AGE applies bounded lowpass + soft saturation; default is exact bypass.
  */
 s->tape_mix_smooth=etch_fx_slew(s->tape_mix_smooth,qclamp(c->tape_mix));
 tape_line[s->tape_pos]=clip16(x);
 if(s->tape_mix_smooth){
  int32_t wow=(int32_t)fx_triangle(s->tape_wow_phase)-32767;
  int32_t flutter=(int32_t)fx_triangle(s->tape_flutter_phase)-32767;
  int32_t delay_q8=180*256;
  delay_q8+=(int32_t)(((int64_t)wow*qclamp(c->tape_wow)*90*256)/(32767*32767));
  delay_q8+=(int32_t)(((int64_t)flutter*qclamp(c->tape_flutter)*15*256)/(32767*32767));
  uint32_t delay=(uint32_t)delay_q8;
  uint16_t rd=(uint16_t)((s->tape_pos-(delay>>8))&511u);
  int32_t a=tape_line[rd],b=tape_line[(rd-1u)&511u];
  int32_t y=(int32_t)((a*(256-(int32_t)(delay&255u))+b*(int32_t)(delay&255u))/256);
  uint32_t age=qclamp(c->tape_age);
  uint32_t alpha=32767u-(age*26000u/32767u);
  s->tape_tone_state+=(int32_t)(((int64_t)(y-s->tape_tone_state)*alpha)/32768);
  y=s->tape_tone_state;
  /* Subtle tape drive with hard numerical bounds and no artificial hiss. */
  if(age){
   int32_t drive=32767u+age/2u;
   int32_t raw=(int32_t)(((int64_t)y*drive)/32767);
   y=clip16(raw);
  }
  x+=(int32_t)(((int64_t)(y-x)*s->tape_mix_smooth)/32767);
 }else s->tape_tone_state=x;
 s->tape_pos=(uint16_t)((s->tape_pos+1u)&511u);
 s->tape_wow_phase+=s->tape_wow_step;
 s->tape_flutter_phase+=s->tape_flutter_step;
 /* Decimator: 4..16 significant bits, hold 1..64 samples. */
 s->decim_mix_smooth=etch_fx_slew(s->decim_mix_smooth,qclamp(c->decim_mix));
 if(s->decim_mix_smooth){
  uint32_t bits=4u+qclamp(c->decim_bits)*12u/32767u;
  uint32_t skip=16u-bits;
  uint32_t hold=1u+qclamp(c->decim_rate)*63u/32767u;
  if(!s->decim_counter){
   int32_t driven=clip16((int32_t)(((int64_t)x*(32767u+qclamp(c->decim_drive)*3u))/32767u));
   s->decim_held=(driven / (int32_t)(1u<<skip))*(int32_t)(1u<<skip);
   s->decim_counter=(uint16_t)hold;
  }
  s->decim_counter--;
  x+=(int32_t)(((int64_t)(s->decim_held-x)*s->decim_mix_smooth)/32767);
 }else s->decim_counter=0;
 /* Wavefolder: bounded symmetric folding, symmetry offset and output tone. */
 s->folder_mix_smooth=etch_fx_slew(s->folder_mix_smooth,qclamp(c->folder_mix));
 if(s->folder_mix_smooth){
  if(!s->folder_baseline_valid || s->folder_last_fold!=c->folder_fold ||
     s->folder_last_symmetry!=c->folder_symmetry){
   s->folder_baseline=fx_fold_transfer(0,c->folder_fold,c->folder_symmetry);
   s->folder_last_fold=c->folder_fold;
   s->folder_last_symmetry=c->folder_symmetry;
   s->folder_baseline_valid=1u;
  }
  int32_t y=(int32_t)fx_fold_transfer(x,c->folder_fold,c->folder_symmetry)-s->folder_baseline;
  y=clip16(y);
  uint32_t alpha=256u+qclamp(c->folder_tone)*32511u/32767u;
  s->folder_tone_state+=(int32_t)(((int64_t)(y-s->folder_tone_state)*alpha)/32767);
  x+=(int32_t)(((int64_t)(s->folder_tone_state-x)*s->folder_mix_smooth)/32767);
 }else s->folder_tone_state=x;
 return clip16(x);
}

void etch_fx_process_stereo_ex(struct etch_native_fx *s,int16_t in,uint16_t chorus,uint16_t delay,uint16_t reverb,const struct etch_fx_settings *cfg,int16_t *left,int16_t *right){
 if(!left||!right)return;
 if(!s){*left=*right=in;return;}
 struct etch_fx_settings defaults;
 if(!cfg){etch_fx_defaults(&defaults);cfg=&defaults;}
 chorus=qclamp(chorus);delay=qclamp(delay);reverb=qclamp(reverb);
 /* V1.10 fast dry: return EXACT samples, not a signal that traversed
  * apparently disabled effects. Skips costly filters, 64-bit multiplies,
  * multiple circular-buffer reads and the serial wet safety knee.
  * Never cut an ongoing wet->dry slew short: all smooth controls must
  * have naturally reached zero, and previous EQ/compression must be unity. */
 if(etch_fx_controls_dry(cfg,chorus,delay,reverb) &&
    !s->reverb_mix_smooth &&
    !s->ring_mix_smooth && !s->trem_mix_smooth && !s->phaser_mix_smooth &&
    !s->flanger_mix_smooth && !s->decim_mix_smooth &&
    !s->folder_mix_smooth && !s->tape_mix_smooth &&
    !s->reverse_mix_smooth && !s->vibrato_mix_smooth &&
    !s->rotary_mix_smooth && !s->wah_mix_smooth &&
    s->eq_gain_smooth[0]==16384 && s->eq_gain_smooth[1]==16384 &&
    s->eq_gain_smooth[2]==16384 && s->comp_gain==32767){
   /* Previously the dry fast-path froze every delay/reverb ring. A later
    * wet preset could replay obsolete audio from the previous patch.
    * Clear one indexed position per sample during dry playback; after
    * 16384 samples (~372 ms @44.1 kHz) every FX ring is flushed.
    * This costs bounded stores and NEVER stalls the DMA IRQ with memset. */
   if(s->dry_was_wet){
    s->dry_was_wet=0u;s->dry_flush_active=1u;s->dry_flush_cursor=0u;
   }
   if(s->dry_flush_active){
    uint32_t i=s->dry_flush_cursor;
    s->delay[i]=0;
    if(i<ETCH_REVERB_SIZE)s->reverb[i]=0;
    if(i<ETCH_CHORUS_SIZE)s->chorus[i]=0;
    if(i<64u)space_diffuser[i]=0;
    if(i<3584u)reverse_line[i]=0;
    if(i<1024u)s->flanger_line[i]=0;
    if(i<512u){vibrato_line[i]=0;tape_line[i]=0;}
    if(++s->dry_flush_cursor==ETCH_DELAY_SIZE){
     s->dry_flush_active=0u;s->dry_flush_cursor=0u;
     /* Small persistent filter states are not allowed to resurrect a tone. */
     s->eq_lp_low=s->eq_lp_high=s->eq_lp_hpf=0;
     s->phaser_feedback_state=s->flanger_feedback_state=0;
     for(uint32_t z=0;z<4u;z++)s->phaser_z[z]=0;
     s->wah_env=s->wah_lp=s->wah_hp=0;
     s->tape_tone_state=s->reverse_tone_state=0;
     s->folder_tone_state=s->folder_baseline=0;
     s->space_damp_state=0;
     s->reverse_accum=0;s->reverse_ready=0;
    }
   }
   *left=in;*right=in;return;
 }
 s->dry_was_wet=1u;s->dry_flush_active=0u;
 /* Host-updated FX settings change at UI rate. Never perform parameter
  * conversion divisions inside the 44.1 kHz audio loop unnecessarily. */
 if(!s->applied_valid||
    s->applied.chorus_rate!=cfg->chorus_rate||s->applied.chorus_depth!=cfg->chorus_depth||
    s->applied.delay_time!=cfg->delay_time||s->applied.delay_feedback!=cfg->delay_feedback||
    s->applied.reverb_size!=cfg->reverb_size||s->applied.reverb_decay!=cfg->reverb_decay||
    s->applied.routing!=cfg->routing||
    s->applied.ring_rate!=cfg->ring_rate||s->applied.eq_hpf!=cfg->eq_hpf||
    s->applied.trem_rate!=cfg->trem_rate||s->applied.phaser_rate!=cfg->phaser_rate||
    s->applied.flanger_rate!=cfg->flanger_rate||s->applied.tape_wow!=cfg->tape_wow||
    s->applied.tape_flutter!=cfg->tape_flutter||
    s->applied.vibrato_rate!=cfg->vibrato_rate||s->applied.rotary_rate!=cfg->rotary_rate){
  s->applied=*cfg;s->applied_valid=1;
  /* 1..1600Hz, gentle resolution around low rates. */
  uint64_t r=qclamp(cfg->ring_rate);
  uint32_t hz=1u+(uint32_t)((r*r*1599u)/(32767u*32767u));
  s->ring_increment=hz*97391u;
  s->trem_step=14609u+(uint32_t)qclamp(cfg->trem_rate)*1450000u/32767u;
  s->phaser_step=2922u+(uint32_t)qclamp(cfg->phaser_rate)*980000u/32767u;
  s->flanger_step=2922u+(uint32_t)qclamp(cfg->flanger_rate)*980000u/32767u; /* round(2^32/44100), bounded */
  s->vibrato_step=19479u+(uint32_t)qclamp(cfg->vibrato_rate)*856020u/32767u; /* .2..9Hz */
  s->rotary_step=19479u+(uint32_t)qclamp(cfg->rotary_rate)*681520u/32767u; /* .2..7.2Hz */
  /* 0.13..1.3 Hz and 3..12 Hz @44.1k, independent modulator clocks. */
  s->tape_wow_step=12659u+(uint32_t)qclamp(cfg->tape_wow)*113700u/32767u;
  s->tape_flutter_step=292153u+(uint32_t)qclamp(cfg->tape_flutter)*876457u/32767u;
  s->eq_hpf_alpha=(uint16_t)(((uint32_t)qclamp(cfg->eq_hpf)*1700u)/32767u);
  s->rate_step=1u+(uint32_t)qclamp(cfg->chorus_rate)*9u/32768u;
  /* Precompute span at UI rate, not with an expensive 64-bit audio multiply. */
  s->chorus_span=(uint16_t)((uint32_t)qclamp(cfg->chorus_depth)*924u/32767u);
  s->space_damp_alpha=(uint16_t)(7000u+(uint32_t)(32767u-qclamp(cfg->reverb_decay))*8000u/32767u);
  s->delay_samples=384u+(uint32_t)qclamp(cfg->delay_time)*15488u/32767u;
  uint32_t rsize=8192u+(uint32_t)qclamp(cfg->reverb_size)*24575u/32767u;
  s->tap1=1559u*rsize/32767u;s->tap2=2707u*rsize/32767u;s->tap3=4019u*rsize/32767u;
  if(!s->tap1)s->tap1=1;
  if(!s->tap2)s->tap2=1;
  if(!s->tap3)s->tap3=1;
 }
 uint16_t feedback=qclamp(cfg->delay_feedback),decay=qclamp(cfg->reverb_decay);
 in=etch_fx_ring_eq(s,in,cfg);
 in=fx_v102_process(s,in,cfg);
 uint32_t phase=s->chorus_phase;
 s->chorus_phase=(phase+s->rate_step)&65535u;
 uint32_t tri=phase<32768u?phase:65535u-phase;
 /* V1.22: opposite L/R modulation, plus interpolated sub-sample reads.
  * The same 2048-sample ring is retained; no second stereo chorus buffer.
  * 264 samples is 6ms; depth adds up to ~21ms. */
 uint32_t shift_l=((uint32_t)tri*s->chorus_span)>>7;
 uint32_t shift_r=((uint32_t)(32767u-tri)*s->chorus_span)>>7;
 int32_t wetc=etch_fx_chorus_read(s,264u*256u+shift_l);
 int32_t wetc_r=etch_fx_chorus_read(s,281u*256u+shift_r);
 s->chorus[s->chorus_pos]=in;
 s->chorus_pos=(s->chorus_pos+1u)&(ETCH_CHORUS_SIZE-1u);
 int32_t chorus_delta=((wetc-in)*(int32_t)chorus)/65536;
 int32_t chorus_right=((wetc_r-in)*(int32_t)chorus)/65536;
 /* Echo spans about 9..360ms @ 44.1kHz. Never read the write head. */
 uint32_t didx=(s->delay_pos-s->delay_samples)&(ETCH_DELAY_SIZE-1u);
 int32_t wetd=s->delay[didx];
 int32_t wetd_right=s->delay[(didx-97u)&(ETCH_DELAY_SIZE-1u)];
 int32_t fxin=cfg->routing==1?in:clip16((int32_t)in+chorus_delta);
 s->delay[s->delay_pos]=clip16(fxin+(int32_t)(((int64_t)wetd*feedback*27)>>20));
 s->delay_pos=(s->delay_pos+1u)&(ETCH_DELAY_SIZE-1u);
 int32_t delay_delta=(wetd*(int32_t)delay)/65536;
 int32_t delay_right=(wetd_right*(int32_t)delay)/65536;
 /* Three size-dependent taps share the original 4096-sample tank. */
 int32_t r1=s->reverb[(s->reverb_pos-s->tap1)&(ETCH_REVERB_SIZE-1u)];
 int32_t r2=s->reverb[(s->reverb_pos-s->tap2)&(ETCH_REVERB_SIZE-1u)];
 int32_t r3=s->reverb[(s->reverb_pos-s->tap3)&(ETCH_REVERB_SIZE-1u)];
 int32_t early_a=s->reverb[(s->reverb_pos-197u)&(ETCH_REVERB_SIZE-1u)];
 int32_t early_b=s->reverb[(s->reverb_pos-467u)&(ETCH_REVERB_SIZE-1u)];
 int32_t diffusion=(r1+r2+r3)/3;
 int32_t input_r=cfg->routing==1?in:clip16(fxin+delay_delta);
 /* One short Schroeder all-pass diffuser before the existing late tank.
  * An input trim keeps feedback saturation controlled; reverb MIX endpoints
  * and the serial/parallel signal path remain unchanged. */
 int32_t diff_in=(input_r*2)/3;
 int32_t diff_prev=space_diffuser[(s->reverb_pos-43u)&63u];
 int32_t diffused=diff_prev-(diff_in/2);
 space_diffuser[s->reverb_pos&63u]=clip16(diff_in+(diff_prev/2));
 /* Feedback damping reduces bright, metallic accumulation at large DECAY.
  * The coefficient changes only when user control settings change. */
 s->space_damp_state+=(int32_t)(((int64_t)(diffusion-s->space_damp_state)*s->space_damp_alpha)/32768);
 s->reverb[s->reverb_pos]=clip16(diffused+(int32_t)(((int64_t)s->space_damp_state*decay*27)>>20));
 s->reverb_pos=(s->reverb_pos+1u)&(ETCH_REVERB_SIZE-1u);
 /* V1.14: REVERB MIX is a true constant-gain dry/wet blend. V1.13
  * always ADDED a half-level diffuse return to a full dry signal, even at
  * 100% MIX. That made 0% and 100% seem almost identical on the FM1.
  * Output at 0 is bit-identical dry; output at 32767 is only the reverb
  * tank return. Do not change saved Q15 settings or flash formats. */
 int32_t dry_left=cfg->routing==1?(int32_t)in+chorus_delta+delay_delta:input_r;
 int32_t dry_right=(int32_t)in+chorus_right+delay_right;
 /* V1.15 SPACE: the old late-only tank could almost cancel periodic
  * oscillator notes, and left/right shared three close taps. Two early
  * reflections make knob motion clearly audible even on short notes.
  * Retain the true dry/wet 0/100 endpoints and the existing decay tank. */
 int32_t wet_left=clip16((early_a*5+early_b*3+r1*4+r2*3+r3*2)/12);
 int32_t wet_right=clip16((early_b*5+early_a*2+r2*4+r3*3+r1*2)/12);
 s->reverb_mix_smooth=etch_fx_slew(s->reverb_mix_smooth,reverb);
 uint16_t space=(uint16_t)s->reverb_mix_smooth;
 int32_t output=(int32_t)(((int64_t)dry_left*(32767u-space)+
                            (int64_t)wet_left*space)/32767);
 int32_t output_right=(int32_t)(((int64_t)dry_right*(32767u-space)+
                                  (int64_t)wet_right*space)/32767);
 /* Reverse delay: alternate recording and reading two disjoint blocks.
  * Entire last block is read backwards while current block is filled.
  * TIME maps 1024..8192 samples (23..186ms at 44.1k).
  * Playback/capture boundaries are guarded by a 128-sample triangular fade.
  * RATE changes only at a new capture block (no mid-block index tear). */
 s->reverse_mix_smooth=etch_fx_slew(s->reverse_mix_smooth,qclamp(cfg->reverse_mix));
 if(!s->reverse_len)s->reverse_len=1024u;
 uint16_t n=s->reverse_len;
 int32_t reverse_wet=0;
 if(s->reverse_mix_smooth && s->reverse_ready){
  uint32_t prev=(uint32_t)(s->reverse_bank^1u)*1792u;
  uint16_t idx=(uint16_t)(n-1u-s->reverse_pos);
  int32_t now=reverse_line[prev+idx];
  int32_t next=reverse_line[prev+(idx?idx-1u:n-1u)];
  int32_t wet=(now*(int32_t)(4u-s->reverse_subphase)+next*s->reverse_subphase)/4;
  uint32_t edge=s->reverse_pos<128u?s->reverse_pos:(n-s->reverse_pos<128u?n-s->reverse_pos:128u);
  wet=(int32_t)(((int64_t)wet*edge)/128u);
  uint32_t alpha=1024u+(uint32_t)qclamp(cfg->reverse_tone)*31743u/32767u;
  s->reverse_tone_state+=(int32_t)(((int64_t)(wet-s->reverse_tone_state)*alpha)/32768);
  reverse_wet=s->reverse_tone_state;
 }
 uint32_t current=(uint32_t)s->reverse_bank*1792u;
 int32_t fb=(int32_t)(((int64_t)reverse_wet*qclamp(cfg->reverse_feed)*19660u)/(32767u*32767u));
 /* Four-sample averaged capture at 11.025kHz extends the maximum reverse
  * length without additional SRAM. Fractional playback between captured
  * samples avoids zipper artifacts; TONE shapes the decimated bandwidth. */
 s->reverse_accum+=output;
 if(++s->reverse_subphase>=4u){
  reverse_line[current+s->reverse_pos]=clip16(s->reverse_accum/4+fb);
  s->reverse_accum=0;s->reverse_subphase=0;
  if(++s->reverse_pos>=n){
   s->reverse_bank^=1u;s->reverse_pos=0;s->reverse_ready=1;
   /* Only change capture length at a completed block boundary. */
   uint16_t desired=(uint16_t)(512u+((uint32_t)qclamp(cfg->reverse_time)*1280u)/32767u);
   if(desired!=s->reverse_len){s->reverse_len=desired;s->reverse_ready=0;s->reverse_tone_state=0;}
  }
 }
 if(s->reverse_mix_smooth){
  output+=(int32_t)(((int64_t)(reverse_wet-output)*s->reverse_mix_smooth)/32767);
  output_right+=(int32_t)(((int64_t)(reverse_wet-output_right)*s->reverse_mix_smooth)/32767);
 }
 /* Rotary: complementary L/R spinning horn tremolo plus softened drive.
  * Global, bounded, mono-compatible by averaging the stereo return. */
 s->rotary_mix_smooth=etch_fx_slew(s->rotary_mix_smooth,qclamp(cfg->rotary_mix));
 if(s->rotary_mix_smooth){
  uint32_t tri=fx_triangle(s->rotary_phase);
  int32_t lfo=(int32_t)tri-32767;
  uint32_t depth=qclamp(cfg->rotary_depth);
  int32_t leftgain=32767+(int32_t)(((int64_t)lfo*depth*3)/(32767*4));
  int32_t rightgain=32767-(int32_t)(((int64_t)lfo*depth*3)/(32767*4));
  int32_t drive=32767+(int32_t)qclamp(cfg->rotary_drive);
  int32_t l=clip16((int32_t)(((int64_t)output*leftgain*drive)/(32767LL*32767LL)));
  int32_t r=clip16((int32_t)(((int64_t)output_right*rightgain*drive)/(32767LL*32767LL)));
  output+=(int32_t)(((int64_t)(l-output)*s->rotary_mix_smooth)/32767);
  output_right+=(int32_t)(((int64_t)(r-output_right)*s->rotary_mix_smooth)/32767);
 }
 s->rotary_phase+=s->rotary_step;
 *left=etch_fx_wet_safety(output,(uint8_t)(chorus||delay||reverb||s->reverse_mix_smooth||s->rotary_mix_smooth));
 *right=etch_fx_wet_safety(output_right,(uint8_t)(chorus||delay||reverb||s->reverse_mix_smooth||s->rotary_mix_smooth));
}
int16_t etch_fx_process_ex(struct etch_native_fx *s,int16_t in,uint16_t chorus,uint16_t delay,uint16_t reverb,const struct etch_fx_settings *cfg){
 int16_t left=in,right=in;
 etch_fx_process_stereo_ex(s,in,chorus,delay,reverb,cfg,&left,&right);
 return left;
}
int16_t etch_fx_process(struct etch_native_fx *s,int16_t in,uint16_t chorus,uint16_t delay,uint16_t reverb){
 return etch_fx_process_ex(s,in,chorus,delay,reverb,0);
}
