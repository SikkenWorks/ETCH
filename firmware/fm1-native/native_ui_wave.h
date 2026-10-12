/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V0.46: bounded pure UI helpers for MORPH and LIBRARY.
 * No GPIO, flash writes, memory allocation or audio callback work.
 */
#pragma once
#include <stdint.h>
/* 32 encoder anchors span the whole physical 240px screen. */
static inline uint32_t etch_ui_draw_cursor_x(uint8_t raw_cursor){
 uint32_t grip=(uint32_t)raw_cursor/4u;
 if(grip>31u)grip=31u;
 return (grip*239u+15u)/31u;
}


static inline uint16_t etch_ui_live_frame_interval(uint8_t voices,uint8_t missed,uint8_t halted){
 /* Target 20 FPS with a bounded DMA canvas. Never throttle on voice count
  * alone: V1.08 dropped to 2.8 FPS simply because 4 voices were active.
  * Actual missed deadlines / ISR pressure retain priority over animation. */
 if(halted)return 750u;
 if(missed>=3u)return 120u;
 if(missed>=1u)return 80u;
 (void)voices;return 50u;
}

/* A bounded rising zero-crossing search gives the scope a repeatable
 * phase under sustained notes. Keep 384 of 512 samples visible. */
/* Track the closest valid rising zero-crossing to the last visible phase.
 * In particular, do not jump to the first crossing at every ARP pitch change.
 * This is UI-only and bounded by count-window, never called in audio ISR. */
static inline uint16_t etch_ui_scope_trigger_near(const int16_t *s,uint16_t count,
                                                  uint16_t window,uint16_t prior){
 if(!s||!window||count<=window)return 0u;
 uint16_t end=(uint16_t)(count-window),best=prior>end?end:prior;
 uint32_t distance=0xffffffffu;uint8_t found=0u;
 for(uint16_t i=1;i<=end;i++){
  if(s[i-1]<0 && s[i]>=0 && (int32_t)s[i]-(int32_t)s[i-1]>24){
   uint32_t d=(uint32_t)(i>prior?i-prior:prior-i);
   if(d<distance){distance=d;best=i;found=1u;}
  }
 }
 return found?best:(prior>end?end:prior);
}
static inline uint16_t etch_ui_scope_trigger(const int16_t *s,uint16_t count,
                                              uint16_t window){
 if(!s||count<=window||window==0u)return 0u;
 uint16_t last=(uint16_t)(count-window);
 for(uint16_t i=1;i<=last;i++)if(s[i-1]<0 && s[i]>=0 && (s[i]-s[i-1])>24)return i;
 return 0u;
}
/* Every live FX knob is included, not just the generic PLAY macros.
 * FNV-1a over fixed 16-bit fields; never called in the audio ISR. */
static inline uint32_t etch_ui_values_fingerprint(const uint16_t *values,uint8_t count){
 uint32_t hash=2166136261u;
 for(uint8_t i=0;i<count;i++){
  hash=(hash^(uint8_t)values[i])*16777619u;
  hash=(hash^(uint8_t)(values[i]>>8))*16777619u;
 }
 return hash;
}
/* Center the full visible capture's vertical bounding box at y=middle.
 * This affects LCD-only samples: never DSP, storage or USB audio.
 * Use 32-bit intermediates because +/-32768 endpoints otherwise overflow.
 * One midpoint per captured frame prevents per-column vertical jitter. */
static inline int32_t etch_ui_scope_midpoint(const int16_t *frame,uint16_t count){
 if(!frame||!count)return 0;
 int32_t lo=32767,hi=-32768;
 for(uint16_t i=0;i<count;i++){
  int32_t x=frame[i];if(x<lo)lo=x;if(x>hi)hi=x;
 }
 return (lo+hi)/2;
}
/* Single pass for both offset and peak. No heap/IRQ work, bounded to the
 * latest 512 captured samples; avoids scanning a full waveform twice. */
static inline void etch_ui_scope_stats(const int16_t *frame,uint16_t count,
                                      int32_t *offset,uint32_t *peak){
 int32_t lo=32767,hi=-32768;uint32_t max_peak=0u;
 if(!frame||!count){if(offset)*offset=0;if(peak)*peak=0;return;}
 for(uint16_t i=0;i<count;i++){
  int32_t x=frame[i];if(x<lo)lo=x;if(x>hi)hi=x;
  uint32_t mag=(uint32_t)(x<0?-x:x);if(mag>max_peak)max_peak=mag;
 }
 if(offset)*offset=(lo+hi)/2;
 if(peak)*peak=max_peak;
}
/* Scope offset glides across frames rather than jumping vertically whenever
 * the note/FX phase changes. The LCD is the only consumer. */
static inline int32_t etch_ui_scope_smooth_center(int32_t previous,int32_t target,
                                                 uint8_t initialise){
 if(initialise)return target;
 int32_t delta=target-previous;
 if(delta>0)return previous+(delta+3)/4;
 if(delta<0)return previous-((-delta+3)/4);
 return previous;
}
static inline int16_t etch_ui_scope_centered(int16_t raw,int32_t offset){
 int32_t x=(int32_t)raw-offset;
 return (int16_t)(x<-32768?-32768:x>32767?32767:x);
}
/* The centered waveform must be autoscaled from its centered extrema,
 * not a different DC origin. Q8, same min/max gain policy as usual. */
static inline uint16_t etch_ui_scope_centered_gain(const int16_t *frame,uint16_t count,int32_t offset){
 uint32_t peak=0;
 if(!frame)return 256u;
 for(uint16_t i=0;i<count;i++){
  int32_t x=(int32_t)frame[i]-offset;
  uint32_t m=(uint32_t)(x<0?-x:x);
  if(m>peak)peak=m;
 }
 if(peak<512u)return 256u;
 uint32_t gain=(29490u*256u)/peak;
 return (uint16_t)(gain<256u?256u:gain>4096u?4096u:gain);
}
/* Q8 LCD-only zoom. The signal has already passed through MASTER.
 * Recover an estimate of the signal before MASTER to compute a stable zoom.
 * Hence turning MASTER down still shrinks the visible waveform instead of
 * the scope amplifying it back up on every new ARP note or FX transient.
 * Hold the previous zoom on silence to prevent attack/rest pumping.
 * All arithmetic is bounded 32-bit; never called in the audio interrupt. */
static inline uint16_t etch_ui_scope_master_zoom(uint32_t peak,
                                                uint16_t master_q15,
                                                uint16_t previous_q8){
 if(previous_q8<256u || previous_q8>2048u)previous_q8=768u;
 if(peak<64u || master_q15<512u)return previous_q8;
 uint32_t premaster=(peak*32767u)/(uint32_t)master_q15;
 if(premaster<1u)premaster=1u;
 uint32_t target=(27000u*256u)/premaster;
 if(target<256u)target=256u;
 if(target>2048u)target=2048u;
 /* Four-frame glide prevents visual jumps without changing synth DSP. */
 return (uint16_t)(((uint32_t)previous_q8*3u+target+2u)/4u);
}
/* Display-only auto-scaling: no effect on oscillator amplitudes, audio DMA,
 * output gain or saved wave data. The threshold rejects near-silence and the
 * bounded gain avoids displaying tiny noise as full-scale sound. Q8 gain. */
static inline uint16_t etch_ui_wave_display_gain(const int16_t *samples,uint16_t count){
 uint32_t peak=0;
 for(uint16_t i=0;i<count;i++){
  int32_t s=samples[i];uint32_t mag=(uint32_t)(s<0?-s:s);
  if(mag>peak)peak=mag;
 }
 if(peak<512u)return 256u;
 uint32_t gain=(29490u*256u)/peak; /* target 90% of Q15 range */
 if(gain<256u)gain=256u;
 if(gain>4096u)gain=4096u;
 return (uint16_t)gain;
}
static inline uint32_t etch_ui_wave_display_y(int16_t sample,uint16_t gain_q8,
                                             uint16_t center,uint16_t extent,
                                             uint16_t min_y,uint16_t max_y){
 /* Divide first to keep the entire operation in safe 32-bit range on pi32v2. */
 int32_t scaled=((int32_t)sample*(int32_t)gain_q8)/256;
 int32_t y=(int32_t)center-(scaled*(int32_t)extent)/32768;
 if(y<min_y)y=min_y;
 if(y>max_y)y=max_y;
 return (uint32_t)y;
}

/* A slot is selected only when its valid bit is set. */
static inline uint8_t etch_ui_wave_valid(const uint32_t valid[2], uint8_t slot){
 return slot<64u && ((valid[slot>>5]>>(slot&31u))&1u)!=0;
}
/* Search for the next stored slot, or retain the selection if none exists. */
static inline uint8_t etch_ui_wave_next(const uint32_t valid[2],uint8_t current,int8_t dir){
 if(!dir)return current<64u?current:0;
 for(uint8_t i=0;i<64u;i++){
  current=(uint8_t)((current+(dir>0?1u:63u))&63u);
  if(etch_ui_wave_valid(valid,current))return current;
 }
 return current;
}
/* The apparent 3,5,7,25... jump is sparse storage, not skipped encoder
 * detents. Display contiguous ordinal N/TOTAL while preserving the real
 * stable slot IDs used by old patches. */
static inline uint8_t etch_ui_wave_count(const uint32_t valid[2]){
 uint8_t n=0;for(uint8_t i=0;i<64u;i++)if(etch_ui_wave_valid(valid,i))++n;return n;
}
static inline uint8_t etch_ui_wave_ordinal(const uint32_t valid[2],uint8_t slot){
 uint8_t n=0;for(uint8_t i=0;i<64u;i++)if(etch_ui_wave_valid(valid,i)){++n;if(i==slot)return n;}
 return 0;
}
/* Shape preview is the UNMODULATED, same-phase A/B carrier, not audio-output
 * metering. Exactly A at 0, exactly B at 32767, Q15 saturation. */
static inline void etch_ui_wave_morph(const int16_t a[128],const int16_t b[128],
                                     uint16_t morph,int16_t out[128]){
 if(morph>32767u)morph=32767u;
 for(uint16_t i=0;i<128u;i++){
  int32_t v=((int32_t)a[i]*(32767-(int32_t)morph)+(int32_t)b[i]*(int32_t)morph);
  /* Round toward zero: preserve exact endpoints and never overflow. */
  out[i]=(int16_t)(v/32767);
 }
}
/* Font is 8 pixels wide and 7 tall. Clip before touching the LCD. */
static inline uint8_t etch_ui_text_capacity(uint32_t x,uint32_t y){
 if(x>=240u||y>233u)return 0;
 uint32_t n=(240u-x)/8u;
 return n>30u?30u:(uint8_t)n;
}

/* 0=invalid slot, 1=armed, 2=confirmed. Any changed selection must
 * clear *armed first. A different page must always arm anew. */
static inline uint8_t etch_ui_wave_load_click(uint8_t *armed,uint8_t *page_armed,
                                               uint8_t page,uint8_t valid){
 if(!valid){*armed=0;return 0;}
 if(!*armed||*page_armed!=page){*armed=1;*page_armed=page;return 1;}
 *armed=0;return 2;
}

/* SELECT navigation: keep all other pages unchanged. */
static inline uint8_t etch_ui_wave_page_cycle(uint8_t page,int8_t direction){
 if(direction==0 || (page!=0&&page!=23&&page!=3&&page!=24))return page;
 if(direction>0)return page==0?23:page==23?3:page==3?24:0;
 return page==0?24:page==24?3:page==3?23:0;
}
