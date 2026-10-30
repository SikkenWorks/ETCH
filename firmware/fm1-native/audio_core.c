/* SPDX-License-Identifier: GPL-3.0-only */
#include "audio_core.h"
#include "native_tables.h"
#include "native_bend.h"
#include "native_poly_headroom.h"
#include "native_mip_smoothing.h"
#include "native_drive_softclip.h"
#include "native_expressive.h"
#include "native_project_codec.h"
#include "native_attack_smoothing_v184.h"
/* Weak host fallbacks keep the reusable DSP source independently testable.
 * The actual FM1 UI supplies these from the existing PJT29 snapshots. */
__attribute__((weak)) const struct etch_fx_settings *etch_group_fx_for_preset(uint8_t preset){
 (void)preset;return 0;
}
__attribute__((weak)) uint8_t etch_group_velocity_for_preset(uint8_t preset){
 (void)preset;return 0u;
}
__attribute__((weak)) const struct etch_preset_dsp *etch_group_dsp_for_preset(uint8_t preset){
 (void)preset;return 0;
}
/* Single FM1 synth instance; keep the evolution clock out of saturated POOL. */
/* Organic per-voice control state in normal RAM: FM1 DSP POOL is full.
 * One runtime audio engine exists on FM1. Reset clears this alongside audio. */
/* V1.32: reuse the previously write-only 'birth' field as the LIVING
 * smoothing state. No extra SRAM/POOL, no additional per-note DSP buffers. */
struct organic_voice_state {uint32_t seed,time;int32_t living_lp,pitch,goal;uint8_t ready,phase_ready;};
/* BSS has ~144 bytes spare; protected NOINIT has ~20 bytes spare.
 * Split precisely seven and one voice to avoid touching POOL/DSP buffers. */
static struct organic_voice_state organic_voice[ETCH_NATIVE_VOICES-1u];
static struct organic_voice_state organic_voice_extra __attribute__((section(".noinit")));
static struct organic_voice_state *organic_at(uint32_t i){
 return i<ETCH_NATIVE_VOICES-1u ? &organic_voice[i] : &organic_voice_extra;
}
/* Exactly 16 bytes in guarded .noinit; always reinitialized before audio. */
static uint16_t living_voice_age[ETCH_NATIVE_VOICES] __attribute__((section(".noinit")));
static uint32_t evolution_phase __attribute__((section(".pool")));
static int32_t evolution_smooth __attribute__((section(".pool")));
/* Separate .bss states: FM1 audio POOL has no room to grow. */
static int32_t filter_cutoff_smooth __attribute__((section(".noinit")));
static int32_t filter_resonance_smooth __attribute__((section(".noinit")));
/* V1.39: drive is a live performance control. Slew it per PCM sample to
 * suppress discontinuities without allocations or filter resets. */
static int32_t drive_smooth __attribute__((section(".noinit")));
/* One target audio engine. Kept in BSS to preserve Felucca POOL boundary. */
static uint16_t preview_mix __attribute__((section(".pool")));
static uint8_t preview_selected __attribute__((section(".pool")));
/* 16 bits for eight Group owners, without growing the saturated audio POOL.
 * Used only in the existing single FM1 audio instance. Explicitly reset. */
static uint16_t voice_group_bits __attribute__((section(".noinit")));
static const struct etch_key_groups *active_groups;
static const struct etch_native_patch *active_group_patches __attribute__((section(".noinit")));
/* Pre-existing 3x18KiB waveform tables from audio_target, shared by
 * main/group 1/group 2 only while GROUPS is on. No new static RAM. */
struct etch_native_bank native_banks[3] __attribute__((section(".pool")));
void etch_native_groups_patch_bank(const struct etch_native_patch *bank){active_group_patches=bank;}
void etch_native_groups_configure(const struct etch_key_groups *g){active_groups=g;}
uint8_t etch_native_voice_group(uint8_t slot){
 return slot<ETCH_NATIVE_VOICES?(uint8_t)((voice_group_bits>>(2u*slot))&3u):255u;
}
uint8_t etch_native_groups_playing(void){
 return (uint8_t)(active_groups && (active_groups->group[0].enabled || active_groups->group[1].enabled));
}
static uint8_t etch_group_for_key(uint8_t key){
 return active_groups?etch_key_group_route(active_groups,key):0u;
}
static const struct etch_native_patch *group_patch_at(uint8_t owner){
 if(!owner||owner>2u||!active_groups||!active_group_patches)return 0;
 const struct etch_key_group *g=&active_groups->group[owner-1u];
 if(!g->enabled||g->preset>=ETCH_PATCH_SLOTS)return 0;
 const struct etch_native_patch *p=&active_group_patches[g->preset];
 return p->valid?p:0;
}
static uint8_t group_patch_slot(uint8_t owner){
 return owner>=1u&&owner<=2u&&active_groups?active_groups->group[owner-1u].preset:0u;
}
static uint32_t group_adsr_step(uint16_t ms){
 if(!ms)return 0x7fff0000u;
 uint32_t samples=(uint32_t)ms*3000u/68u;
 return samples?0x7fff0000u/samples:0x7fff0000u;
}
static int32_t clamp15(int32_t v){return v<-32768?-32768:v>32767?32767:v;}
static int32_t slew(int32_t a,int32_t b){int32_t d=b-a;return a+(d/64)+(d>0&&d<64?1:d<0&&d>-64?-1:0);}
static int32_t readwave(const struct etch_native_bank *bank,uint32_t w,uint32_t p,uint32_t inc){
 static const uint32_t limit[5]={30198988u,60397977u,120795955u,241591910u,966367641u};
 uint32_t level=0,i=p>>23;int32_t f=(p>>8)&32767u;
 while(level<5&&inc>limit[level])++level;
 const int16_t *t=bank?bank->table[w][level]:etch_tables[w][level];
 int32_t a=t[i]+((int32_t)(t[(i+1)&511]-t[i])*f)/32768;
 /* Blend just below each harmonic-mip boundary instead of abruptly
  * replacing the oscillator table on the next pitch-bend sample.
  * Only the narrow boundary zone pays for a second table lookup.
  * Retain the existing table bank and unmodified stored waveforms. */
 if(level<5u){
  uint32_t alpha=etch_mip_boundary_alpha(inc,limit[level]);
  if(alpha){
   const int16_t *next=bank?bank->table[w][level+1u]:etch_tables[w][level+1u];
   int32_t b=next[i]+((int32_t)(next[(i+1)&511]-next[i])*f)/32768;
   a+=(int32_t)(((int64_t)(b-a)*alpha)/32768);
  }
 }
 return a;
}
/* V1.32: mip selection and boundary alpha depend on NOTE INCREMENT,
 * not phase. Previously readwave recomputed a 64-bit quotient for every
 * carrier/table lookup and every PCM sample near mip boundaries. Prepare
 * these at the existing 32-sample control tick, then only interpolate in
 * the audio inner loop. All six mip levels and boundary fades are preserved. */
struct etch_mip_lookup {uint16_t alpha;uint8_t level;};
static struct etch_mip_lookup mip_prepare(uint32_t inc){
 static const uint32_t limit[5]={30198988u,60397977u,120795955u,241591910u,966367641u};
 struct etch_mip_lookup l={0u,0u};
 while(l.level<5u && inc>limit[l.level])l.level++;
 if(l.level<5u)l.alpha=(uint16_t)etch_mip_boundary_alpha(inc,limit[l.level]);
 return l;
}
static inline int32_t readwave_prepared(const struct etch_native_bank *bank,
 uint32_t w,uint32_t phase,const struct etch_mip_lookup *mp){
 uint32_t i=phase>>23;int32_t frac=(int32_t)((phase>>8)&32767u);
 const int16_t *a=bank?bank->table[w][mp->level]:etch_tables[w][mp->level];
 int32_t x=a[i]+((int32_t)(a[(i+1u)&511u]-a[i])*frac)/32768;
 if(mp->alpha){
  const int16_t *b=bank?bank->table[w][mp->level+1u]:etch_tables[w][mp->level+1u];
  int32_t next=b[i]+((int32_t)(b[(i+1u)&511u]-b[i])*frac)/32768;
  x+=(int32_t)(((int32_t)(next-x)*(int32_t)mp->alpha)/32768);
 }
 return x;
}
/* The V1.31 fundamental-only reference required TWO additional 512-point
 * wavetable interpolations PER VOICE PER SAMPLE, even in dense chords.
 * Replace these with one bounded, pitch-aware one-pole harmonic reference.
 * A single multiply+shift follows the voice's *already computed* carrier;
 * the state lives in an otherwise unused field of organic_voice_state.
 * This also avoids doubling the extra work during old-bank crossfades. */
static inline int16_t living_shape(int32_t full,int32_t *low_state,
 uint32_t inc,uint16_t weight,uint8_t color,uint8_t enabled){
 if(!enabled){*low_state=full;return (int16_t)full;}
 uint32_t alpha=inc>>13; /* follow fundamental; lower notes are darker */
 if(alpha<256u)alpha=256u;
 if(alpha>24576u)alpha=24576u;
 int32_t low=*low_state;
 low+=(int32_t)(((int32_t)(full-low)*(int32_t)alpha)>>15);
 *low_state=low;
 if(!weight)return (int16_t)full;
 int32_t tilt=(int32_t)color*2-100;
 int32_t strength=((int32_t)weight*tilt)/100;
 int32_t delta=((full-low)*strength)>>15;
 return (int16_t)clamp15(full+delta);
}
void etch_native_reset(struct etch_native_audio *s){
 uint8_t *p=(uint8_t*)s;for(uint32_t i=0;i<sizeof(*s);++i)p[i]=0;voice_group_bits=0u;evolution_phase=0u;evolution_smooth=0;
 for(uint32_t oi=0;oi<ETCH_NATIVE_VOICES;oi++){
  living_voice_age[oi]=0u;
  uint8_t *q=(uint8_t*)organic_at(oi);for(uint32_t b=0;b<sizeof(struct organic_voice_state);b++)q[b]=0;
 }preview_mix=0u;preview_selected=0u;filter_cutoff_smooth=0;filter_resonance_smooth=0;drive_smooth=0;s->voice_limit=8u;s->poly_div_q10=4096;for(uint32_t i=0;i<16;i++){s->bend[i]=65536;}
 /* Critical audio regression fix: the previous reset only zeroed the FX
  * struct, but NEVER called the FX initializer. That left the EQ smoothed
  * gains at 0 (instead of unity), compressor gain at 0, and three .noinit
  * delay lines containing stale power-on RAM. Initialize once on a stopped
  * audio path, never inside the render callback or during patch changes. */
 etch_fx_reset(&s->fx);
 /* Cold boot always starts with no synth notes and a closed digital
  * silence gate: any effect DC or stale feedback is inaudible until a
  * real note is played. No hardware DAC/USB switching is involved. */
 s->idle_samples=132300u;s->idle_gain=0;
}
/* Advanced, deterministic allocation. A note-on never grows the DSP voice
 * count: free > released > sustained > matching retrigger > quiet active.
 * A loud audible bass/top voice receives a modest protection bonus, while
 * equal-priority/equal-level voices always break ties by oldest birth.
 * Allocation is bounded O(8), never called in the sample loop. */
static uint32_t on(struct etch_native_audio *s,uint8_t pitch,uint16_t identity){
 uint32_t limit=s->mono_mode?1u:(etch_voice_budget_valid(s->voice_limit)?s->voice_limit:8u);
 uint32_t slot=limit,oldest=0;
 uint8_t best_rank=255;
 uint32_t best_importance=0xffffffffu;
 uint32_t low=0xffffffffu,high=0u;
 for(uint32_t j=0;j<limit;j++){
  const struct etch_native_voice *v=&s->voice[j];
  if(!v->active){slot=j;break;}
  if(v->held && v->down){if(v->inc<low)low=v->inc;if(v->inc>high)high=v->inc;}
 }
 if(slot==limit){
  for(uint32_t j=0;j<limit;j++){
   const struct etch_native_voice *v=&s->voice[j];
   uint8_t rank=!v->held || v->stage==3 ? 0u:
                 !v->down ? 1u:
                 v->note==identity ? 2u:3u;
   uint32_t level=v->env>0?(uint32_t)v->env:0u;
   /* Velocity and *measured* envelope, not pitch alone, define audibility. */
   uint32_t importance=level*(uint32_t)(v->velocity?v->velocity:1u)/127u;
   if(rank==3u && importance && low<high && (v->inc==low || v->inc==high))
    importance+=importance/2u; /* soft protection, never an unstealable voice */
   uint32_t age=s->alloc_serial-v->birth;
   if(rank<best_rank || (rank==best_rank &&
       (importance<best_importance || (importance==best_importance && age>oldest)))){
    slot=j;best_rank=rank;best_importance=importance;oldest=age;
   }
  }
 }
 if(slot>=limit)slot=0;
 /* Bind ownership after stealing, before the new note becomes active.
  * Its MIDI identity remains unchanged for FIFO note-off and sustain. */
 uint16_t mask=(uint16_t)(3u<<(2u*slot));
 uint16_t bits=(uint16_t)(etch_group_for_key(pitch)<<(2u*slot));
 voice_group_bits=(uint16_t)((voice_group_bits&~mask)|bits);
 struct etch_native_voice *v=&s->voice[slot];
 int32_t tail=v->active?v->last:0;
 if(v->active && s->steal<255u)++s->steal;
 /* ONE SHOT Evolution restarts with the first note of a new phrase,
  * not every later note added to a chord. Later note-ons must not reset
  * an already audible wave shaper (click / zipper prevention). */
 uint8_t phrase_start=1u;
 for(uint32_t k=0;k<limit;k++)if(s->voice[k].active&&s->voice[k].held){phrase_start=0u;break;}
 if(phrase_start)evolution_phase=0u;
 v->gate_left=0;v->birth=++s->alloc_serial;v->p=v->q=v->eg=0;
 living_voice_age[slot]=0u;
 struct organic_voice_state *org=organic_at(slot);
 org->seed=etch_organic_hash(v->birth ^ ((uint32_t)identity<<16) ^ (uint32_t)pitch);
 org->time=0u;org->pitch=org->goal=0;org->ready=org->phase_ready=0;org->living_lp=0;
 v->low=v->band=0;for(uint32_t w=0;w<2u;w++)v->pre_lp[w]=0;v->stage=0;v->env=0;v->note=identity;
 v->inc=etch_note_inc[pitch];v->active=v->held=v->down=1;
 v->velocity=127;v->last=0;v->tail=tail;v->fade=tail?ETCH_TRANSITION_SAMPLES:0;v->attack_fade=ETCH_ATTACK_SAMPLES;
 return slot;
}

/* V1.00 MONO: last-note priority including fall-back when the most recent
 * key is released. FIFO note-off for repeated same-pitch MIDI messages.
 * The stack owns key identity; voice[0] is merely the one audible oscillator.
 * All operations are bounded and run only on input edges / step boundaries. */
static void mono_resume(struct etch_native_audio *s){
 struct etch_native_voice *v=&s->voice[0];
 if(!s->mono_count){v->down=0;v->held=0;v->gate_left=0;return;}
 const struct etch_mono_held *top=&s->mono_stack[s->mono_count-1u];
 if(v->active && v->held && v->note==top->id && v->inc==etch_note_inc[top->pitch]){
  v->down=top->down;v->gate_left=top->gate_left;return;
 }
 on(s,top->pitch,top->id);
 v->velocity=top->velocity;v->down=top->down;
 v->gate_left=top->gate_left;
}
static void mono_push(struct etch_native_audio *s,uint8_t pitch,uint16_t id,
                      uint8_t velocity,uint32_t gate){
 if(s->mono_count==ETCH_MONO_STACK){
  for(uint8_t i=1;i<ETCH_MONO_STACK;i++)s->mono_stack[i-1u]=s->mono_stack[i];
  --s->mono_count;
 }
 struct etch_mono_held *m=&s->mono_stack[s->mono_count++];
 m->id=id;m->pitch=pitch;m->velocity=velocity?velocity:1u;
 m->gate_left=gate;m->down=1u;
 /* A repeated note-on MUST retrigger even if its MIDI identity and pitch
  * match the current voice. mono_resume only short-circuits fallback. */
 on(s,pitch,id);s->voice[0].velocity=m->velocity;s->voice[0].gate_left=gate;

}
static void mono_release(struct etch_native_audio *s,uint16_t id,uint8_t sustain){
 for(uint8_t i=0;i<s->mono_count;i++){
  if(s->mono_stack[i].id!=id || !s->mono_stack[i].down)continue;
  if(sustain){s->mono_stack[i].down=0u;mono_resume(s);return;}
  for(uint8_t j=i+1u;j<s->mono_count;j++)s->mono_stack[j-1u]=s->mono_stack[j];
  --s->mono_count;mono_resume(s);return;
 }
}
static void mono_drop_channel(struct etch_native_audio *s,uint8_t ch,uint8_t all){
 uint8_t out=0;
 for(uint8_t i=0;i<s->mono_count;i++){
  struct etch_mono_held m=s->mono_stack[i];
  if(m.id>=128u&&m.id<2176u&&(m.id-128u)/128u==ch && (all||!m.down))continue;
  s->mono_stack[out++]=m;
 }
 if(out!=s->mono_count){s->mono_count=out;mono_resume(s);}
}
static void mono_perf_clear(struct etch_native_audio *s){
 uint8_t out=0;
 for(uint8_t i=0;i<s->mono_count;i++)if(s->mono_stack[i].id<4096u)
  s->mono_stack[out++]=s->mono_stack[i];
 if(out!=s->mono_count){s->mono_count=out;mono_resume(s);}
}
static uint8_t mono_perf_tick(struct etch_native_audio *s){
 uint8_t out=0,changed=0;
 for(uint8_t i=0;i<s->mono_count;i++){
  struct etch_mono_held m=s->mono_stack[i];
  if(m.id>=4096u && m.gate_left && !--m.gate_left){changed=1;continue;}
  s->mono_stack[out++]=m;
 }
 if(out!=s->mono_count){s->mono_count=out;mono_resume(s);}
 return changed;
}
void etch_native_midi_panic(struct etch_native_audio *s){
 s->sustain=0;s->mono_count=0;for(uint32_t i=0;i<16;i++){s->bend[i]=65536;}
 for(uint32_t i=0;i<8;i++)if(s->voice[i].note>=128){s->voice[i].held=s->voice[i].down=0;}
}
void etch_native_midi(struct etch_native_audio *s,uint8_t status,uint8_t d1,uint8_t d2){
 if(!s)return;
 if(status==0xf8){
  uint32_t samples=s->clock_samples; s->clock_samples=0;
  if(samples>100&&samples<73500){
   s->clock_sum+=samples;if(++s->clock_count>=24){
    uint32_t avg=s->clock_sum/s->clock_count;
    uint32_t bpm=avg?(uint32_t)(44100u*60u/(24u*avg)):0;
    if(bpm>=30&&bpm<=300)s->clock_bpm=(uint8_t)(bpm>255?255:bpm);
    s->clock_sum=0;s->clock_count=0;
   }
  }else{s->clock_sum=0;s->clock_count=0;}
  return;
 }
 if(status==0xfa){s->clock_running=1;s->perf_step=0;s->perf_clock=0;return;}
 if(status==0xfb){s->clock_running=1;return;}
 if(status==0xfc){s->clock_running=0;s->perf_last_seq=0;return;}
 if(status<0x80||status>=0xf0||d1>127||d2>127)return;
 uint32_t ch=status&15,kind=status&0xf0;uint16_t id=(uint16_t)(128+ch*128+d1);
 if(kind==0xd0){s->bend[ch]=(s->bend[ch]&0x00ffffffu)|((uint32_t)d1<<24);return;}
 if(kind==0xe0){uint32_t value=d1+((uint32_t)d2<<7);s->bend[ch]=(s->bend[ch]&0xff000000u)|etch_bend_q16[(value+64)>>7];return;}
 if(kind==0x80||(kind==0x90&&!d2)){
  if(s->mono_mode){mono_release(s,id,(uint8_t)((s->sustain>>ch)&1u));return;}
  /* Pair one note-off with the OLDEST still-depressed note-on instance.
   * This is FIFO for overlapping same-pitch attacks, independently per MIDI
   * channel; sustain-pedal notes have down=0 and are not matched again.
   * The 8-voice allocator may have stolen an old instance already. */
  uint32_t found=ETCH_NATIVE_VOICES,oldest=0;
  for(uint32_t i=0;i<ETCH_NATIVE_VOICES;i++){
   const struct etch_native_voice *v=&s->voice[i];
   if(v->active&&v->down&&v->note==id){
    uint32_t age=s->alloc_serial-v->birth;
    if(found==ETCH_NATIVE_VOICES||age>oldest){found=i;oldest=age;}
   }
  }
  if(found!=ETCH_NATIVE_VOICES){
   struct etch_native_voice *v=&s->voice[found];v->down=0;
   if(!(s->sustain&(1u<<ch)))v->held=0;
  }
 }else if(kind==0x90){
  /* Preserve previous same-pitch attacks as separate voice instances.
   * Their note-off matching is FIFO and bounded by the 8-voice allocator. */
  uint8_t tuned=etch_quantize_note(d1,&s->harmony);
  if(s->mono_mode){mono_push(s,tuned,id,d2,0u);return;}
  uint32_t i=on(s,d1,id);s->voice[i].inc=etch_note_inc[tuned];s->voice[i].velocity=d2;
 }else if(kind==0xb0){
  if(d1==64){if(d2>=64)s->sustain|=(uint16_t)(1u<<ch);else s->sustain&=(uint16_t)~(1u<<ch);}
  if(d1==121){s->sustain&=(uint16_t)~(1u<<ch);s->bend[ch]=65536;}
  if(s->mono_mode){
   if(d1==120 || d1==123)mono_drop_channel(s,(uint8_t)ch,1u);
   else if((d1==64&&d2<64)||d1==121)mono_drop_channel(s,(uint8_t)ch,0u);
   return;
  }
  for(uint32_t i=0;i<8;i++){
   struct etch_native_voice *v=&s->voice[i];if(v->note<128||(v->note-128)/128!=ch)continue;
   if(d1==120){v->active=v->held=v->down=0;v->env=v->last=v->tail=0;v->fade=0;}
   else if(d1==123){v->down=0;if(!(s->sustain&(1u<<ch)))v->held=0;}
   else if((d1==64&&d2<64)||d1==121){if(!v->down)v->held=0;}
  }
 }
}
/* Q31 envelope accumulators keep long stages precise without division in IRQ. */
static void envelope_with_steps(struct etch_native_voice *v,uint32_t attack,uint32_t decay,
  uint32_t release,uint16_t sustain){
 const uint32_t top=0x7fff0000u;uint32_t target=(uint32_t)(sustain>32767?32767:sustain)<<16;
 if(!v->held)v->stage=3;
 if(v->stage==0){uint32_t k=attack?attack:top;if(k>=top-v->eg){v->eg=top;v->stage=1;}else v->eg+=k;}
 else if(v->stage==1){uint32_t k=decay?decay:top;if(v->eg<=target||k>=v->eg-target){v->eg=target;v->stage=2;}else v->eg-=k;}
 else if(v->stage==2)v->eg=target;
 else {
  /* Zero/near-zero release previously dropped from full amplitude to 0 in
   * one PCM sample. Limit fastest release to >=128 samples (2.9 ms) while
   * preserving every user setting longer than this small safety floor. */
  uint32_t k=release?release:top;
  if(k>top/128u)k=top/128u;
  v->eg=k>=v->eg?0:v->eg-k;
 }
 v->env=(int32_t)(v->eg>>16);
}
static int32_t state_bound(int32_t v){return v<-262144?-262144:v>262144?262144:v;}
/* Bounded Chamberlin SVF, two iterations/sample; coefficients <= 0.25.
   Filter disabled is a true bypass. Saturate state before feedback. */
/* V1.45 exact Q15 filter multiply with no 64-bit divide.
 * Inputs: -262144 <= state <= 262144 and 0 <= coefficient <= 65536.
 * q and r have the same sign, so C truncation-toward-zero is identical to
 * ((int64_t)state*coefficient)/32768. |r*coefficient| <= 2147418112,
 * therefore the 32-bit intermediate cannot overflow. */
static inline int32_t filter_mul_q15(int32_t state,int32_t coefficient){
 int32_t q=state/32768;
 int32_t r=state%32768;
 return q*coefficient+(r*coefficient)/32768;
}
static int32_t filter(struct etch_native_voice *v,const struct etch_native_controls *c,int32_t x){
 if(!c->filter_type){v->low=v->band=0;return x;}
 uint32_t f=c->cutoff>8192?8192:c->cutoff;
 uint32_t res=c->resonance>32767?32767:c->resonance;
 int32_t damping=65536-(int32_t)res*3/2,high=0;
 for(uint32_t k=0;k<2;k++){
  v->low=state_bound(v->low+(int32_t)(filter_mul_q15(v->band,(int32_t)f)));
  high=state_bound(x-v->low-(int32_t)(filter_mul_q15(v->band,damping)));
  v->band=state_bound(v->band+(int32_t)(filter_mul_q15(high,(int32_t)f)));
 }
 return clamp15(c->filter_type==2?v->band:c->filter_type==3?high:v->low);
}
/* Each Wave A/B has two independent, bounded pre-morph inserts.
 * Time-based MASTER effects remain global: allocating e.g. a full reverb
 * for each waveform on every voice would exhaust FM1 SRAM. */
static int32_t pre_insert(struct etch_native_voice *v,const struct etch_fx_settings *fx,
                         uint32_t wave,uint32_t slot,int32_t x,uint32_t phase,int32_t c_sample){
 uint32_t typ=etch_pre_type(fx,(uint8_t)wave,(uint8_t)slot);int32_t mix=etch_pre_mix(fx,(uint8_t)wave,(uint8_t)slot);
 if(!typ||!mix)return x;
 int32_t depth=etch_pre_depth(fx,(uint8_t)wave,(uint8_t)slot),wet=x;
 switch(typ){
 case 1: { /* Ring mod with the exact user-drawn C waveform */
  int32_t amount=(c_sample*depth)/32767;
  wet=(x*amount)/32767;
  break;
 }
 case 2: { /* Bit reduction 16 -> 3 bits, no resampling/block holds */
  uint32_t shift=3u+(uint32_t)(depth*10u/32767u);
  int32_t step=(int32_t)(1u<<shift);
  wet=(x/step)*step;break;
 }
 case 3: { /* Symmetric wavefold, saturated before morphing */
  int32_t hot=(int32_t)(((int64_t)x*(32767u+depth*7u))/32767u);
  while(hot>32767 || hot<-32767){if(hot>32767)hot=65534-hot;else hot=-65534-hot;}
  wet=hot;break;
 }
 case 4: { /* Oscillator-cycle tremolo, no additional phase accumulator */
  int32_t tri=(int32_t)((phase>>16)&65535u);
  tri=tri<32768?tri*2:131070-tri*2;
  int32_t gain=32767-(int32_t)(((int64_t)depth*(32767-tri))/32767);
  wet=(x*gain)/32767;break;
 }
 case 5: { /* Each voice/wave/insert has its OWN LP state */
  int32_t alpha=512+(32767-depth)/2;
  int32_t state=v->pre_lp[wave];
  state+=(int32_t)(((int64_t)(x-state)*alpha)/32768);
  v->pre_lp[wave]=state;wet=state;break;
 }
 default: return x;
 }
 return clamp15(x+(int32_t)(((int64_t)(wet-x)*mix)/32767));
}
/* V1.45: FX insert configuration is stable for one audio block. Evaluate
 * these eight small routing fields once per block/part, not once for every
 * voice and every PCM sample. No parameter smoothing, state, or DSP math
 * changes: a disabled insert still follows the exact original bypass. */
static uint8_t pre_insert_enabled(const struct etch_fx_settings *fx){
 if(!fx)return 0u;
 return (uint8_t)((etch_pre_type(fx,0u,0u)&&etch_pre_mix(fx,0u,0u))||
   (etch_pre_type(fx,0u,1u)&&etch_pre_mix(fx,0u,1u))||
   (etch_pre_type(fx,1u,0u)&&etch_pre_mix(fx,1u,0u))||
   (etch_pre_type(fx,1u,1u)&&etch_pre_mix(fx,1u,1u)));
}
static int32_t carrier(struct etch_native_voice *v,const struct etch_fx_settings *fx,
 const struct etch_native_bank *bank,uint32_t p,uint32_t q,uint32_t inc,uint32_t modinc,int32_t morph,int32_t depth,uint8_t target,uint16_t evolution_amount,uint8_t evolution_mode,
 const struct etch_mip_lookup *carrier_mip,const struct etch_mip_lookup *mod_mip,
 uint8_t pre_active,uint8_t amp_mode,int32_t *amp_signal){
 if(depth){
  int32_t mod=readwave_prepared(bank,2,q,mod_mip);
  if(target){morph=clamp15(morph+(int32_t)(((int32_t)mod*(int32_t)depth)/32768));if(morph<0)morph=0;}
  else p+=((uint32_t)mod*(uint32_t)depth*2u);
 }
 int32_t a=readwave_prepared(bank,0,p,carrier_mip),b=readwave_prepared(bank,1,p,carrier_mip);
 /* Reuse samples ALREADY fetched for the audible carrier. Wave AMP does not
  * allocate a phase accumulator or run an extra wavetable interpolation. */
 if(amp_signal)*amp_signal=etch_wamp_source(amp_mode,a,b,(uint16_t)morph);
 if(pre_active){
  int32_t c_sample=readwave_prepared(bank,2,q,mod_mip);
  for(uint32_t i=0;i<2;i++){
   a=pre_insert(v,fx,0,i,a,p,c_sample);
   b=pre_insert(v,fx,1,i,b,p,c_sample);
  }
 }
 int16_t result=(int16_t)clamp15(a+(int32_t)(((int32_t)(b-a)*(int32_t)morph)/32768));
 return etch_evolution_shape(result,evolution_amount,evolution_mode);
}
/* Audition is not a patch edit: direct waveform + only its own inserts. */
static int32_t preview_carrier(struct etch_native_voice *v,const struct etch_native_controls *c,
 const struct etch_native_bank *bank,uint32_t p,uint32_t q,uint32_t inc,uint32_t modinc,uint8_t wave,const struct etch_mip_lookup *carrier_mip,
 const struct etch_mip_lookup *mod_mip){
 uint8_t w=(uint8_t)(wave-1u);
 int32_t x=readwave_prepared(bank,w,p,carrier_mip);
 if(w<2u){
  const struct etch_fx_settings *fx=&c->fx_settings;
  if((etch_pre_type(fx,w,0u)&&etch_pre_mix(fx,w,0u))||
     (etch_pre_type(fx,w,1u)&&etch_pre_mix(fx,w,1u))){
   int32_t cv=readwave_prepared(bank,2,q,mod_mip);
   x=pre_insert(v,fx,w,0u,x,p,cv);
   x=pre_insert(v,fx,w,1u,x,p,cv);
  }
 }
 return x;
}
/* NOTE: performance-only identifiers are outside the MIDI channel ID range. */
static void perf_off(struct etch_native_audio *s){
 if(s->mono_mode){mono_perf_clear(s);return;}
 for(uint32_t i=0;i<ETCH_NATIVE_VOICES;i++)if(s->voice[i].note>=4096u){s->voice[i].held=s->voice[i].down=0;}
}
static void perf_on(struct etch_native_audio *s,uint8_t note,uint8_t velocity,uint32_t samples){
 if(note>127)return;
 note=etch_quantize_note(note,&s->harmony);
 if(s->mono_mode){mono_push(s,note,(uint16_t)(4096u+note),velocity,samples?samples:1u);return;}
 uint32_t v=on(s,note,(uint16_t)(4096u+note));
 s->voice[v].velocity=velocity?velocity:1;s->voice[v].gate_left=samples?samples:1;
}
/* Held order tracks panel-key press edges, including legato. */
static void perf_order(struct etch_native_audio *s,uint32_t changed,uint32_t keys){
 for(uint8_t k=0;k<27;k++)if(changed&(1u<<k)){
  uint8_t at=0;while(at<s->perf_held_count&&s->perf_order[at]!=k)at++;
  if(at<s->perf_held_count){for(uint8_t j=at;j+1<s->perf_held_count;j++)s->perf_order[j]=s->perf_order[j+1];--s->perf_held_count;}
  if((keys&(1u<<k))&&s->perf_held_count<27)s->perf_order[s->perf_held_count++]=k;
 }
}
static uint8_t arp_pick(struct etch_native_audio *s,const struct etch_native_controls *c,uint32_t keys){
 uint8_t held[27];uint32_t count=0;
 for(uint32_t k=0;k<27;k++)if(keys&(1u<<k))held[count++]=(uint8_t)k;
 if(!count)return 255;
 if(c->arp_mode==4){count=0;for(uint32_t k=0;k<s->perf_held_count;k++)if(keys&(1u<<s->perf_order[k]))held[count++]=s->perf_order[k];if(!count)return 255;}
 if(c->arp_mode==7u){
  int32_t base=53+(int32_t)held[0]+12*c->octave;
  if(base<0)base=0;if(base>127)base=127;
  return etch_riff_pick_variation(c->riff_seed,c->riff_variant,c->riff_mutation,c->riff_density,
    s->perf_index++,(uint8_t)base,&c->harmony,c->arp_octaves);
 }
 uint32_t octaves=c->arp_octaves;
 /* Negative range includes the original octave: -1 = [-1,0],
  * -2 = [-2,-1,0]. Existing positive ranges stay unchanged. */
 int32_t first_octave=0;
 if(octaves==5u){octaves=2u;first_octave=-1;}
 else if(octaves==6u){octaves=3u;first_octave=-2;}
 if(!octaves||octaves>4u)octaves=1;
 uint32_t total=count*octaves,idx=s->perf_index++;
 if(c->arp_mode==6){uint8_t next=etch_arp_program_pick(c->arp_program,idx,total);if(next==255)return 255;idx=next;}
 else if(c->arp_mode==5){s->random_seed=s->random_seed*1664525u+1013904223u;idx=s->random_seed%total;}
 else if(c->arp_mode==3&&total>1){uint32_t cycle=total*2-2;idx%=cycle;if(idx>=total)idx=cycle-idx;}
 else idx%=total;
 uint32_t octave=idx/count,k=idx%count;
 if(c->arp_mode==2)k=count-1-k;
 int32_t note=53+(int32_t)held[k]+12*c->octave+12*((int32_t)octave+first_octave);
 return (uint8_t)(note<0?0:note>127?127:note);
}
static uint32_t perf_period(uint8_t bpm,uint8_t rate){
 uint32_t tempo=bpm<30?120:bpm;uint32_t div=rate>3?8u:(1u<<rate);
 uint32_t n=(44100u*60u)/(tempo*div);return n?n:1u;
}
static uint8_t perf_tick(struct etch_native_audio *s,const struct etch_native_controls *c,uint32_t keys){
 uint8_t pitch_changed=s->mono_mode?mono_perf_tick(s):0u;
 /* Gate countdowns are per voice: a new step never cuts off a long note. */
 for(uint32_t i=0;i<ETCH_NATIVE_VOICES;i++){
  struct etch_native_voice *v=&s->voice[i];
  if(v->active&&v->note>=4096&&v->gate_left){if(!--v->gate_left)v->held=v->down=0;}
 }
 uint8_t seq=c->seq_run!=0&&(!c->clock_follow||s->clock_running);
 uint8_t arp=(!seq&&c->arp_mode&&keys);
 uint8_t was_active=(uint8_t)(s->perf_last_seq||s->perf_last_arp);

 /* ARP7 K4: restart the new riff on the very next audio block, not after
  * the previous long gate/clock completes. random_seed is reused as the
  * last-published riff seed ONLY in ARP7; in RANDOM ARP5 it remains its
  * original LCG state. This adds no bytes to the full audio RAM pool. */
 if(arp && c->arp_mode==7u && s->random_seed!=c->riff_seed){
  s->random_seed=c->riff_seed;
  s->perf_index=0u;
  s->perf_clock=0u;
  s->perf_gate=0u;
 }
 if((seq!=s->perf_last_seq)||(arp!=s->perf_last_arp)){
  perf_off(s);s->perf_clock=0;s->perf_gate=0;s->perf_index=0;s->perf_step=0;
  s->perf_played=0u;s->perf_current_step=0u;pitch_changed=1u;
 }
 s->perf_last_seq=seq;s->perf_last_arp=(uint8_t)(arp!=0);
 if(!seq&&!arp){if(was_active||s->perf_gate){perf_off(s);s->perf_gate=0;}s->seq_lock_mask=0;return pitch_changed;}
 if(s->perf_clock){--s->perf_clock;if(arp&&s->perf_gate&&!--s->perf_gate)perf_off(s);return pitch_changed;}
 if(arp)perf_off(s);
 uint8_t bpm=(c->clock_follow&&s->clock_bpm)?s->clock_bpm:c->tempo;
 uint32_t period=perf_period(bpm,seq?c->seq_rate:c->arp_rate);
 if(seq&&c->seq_swing){
  uint32_t swing=(period*(uint32_t)c->seq_swing)/300u;
  period=(s->perf_step&1u)?(period>swing?period-swing:1):period+swing;
 }
 s->perf_clock=period-1;
 s->perf_gate=(uint32_t)(((uint64_t)period*(seq?(c->seq_gate?c->seq_gate:75u):
                        (c->arp_gate?c->arp_gate:72u)))/100u);
 if(!s->perf_gate)s->perf_gate=1;
 if(seq){
  uint16_t length=(c->seq_length==8||c->seq_length==16||c->seq_length==32||c->seq_length==64||c->seq_length==128||c->seq_length==256)?c->seq_length:16;
  uint16_t step=s->perf_step;
  s->perf_current_step=(uint8_t)step;
  s->perf_played=1u;
  if(c->pattern){
   const struct etch_seq_step *entry=&c->pattern->steps[step];
   s->seq_lock_mask=entry->lock_mask&63u;
   for(uint8_t k=0;k<ETCH_SEQ_LOCKS;k++)s->seq_locks[k]=entry->locks[k];
   uint8_t count=entry->count>ETCH_SEQ_NOTES?ETCH_SEQ_NOTES:entry->count;
   for(uint8_t n=0;n<count;n++){
    const struct etch_seq_note *q=&entry->notes[n];if(q->note>127||!q->velocity)continue;
    uint32_t span=q->length?q->length:1u;
    uint32_t authored=etch_seq_hold_span(c->pattern,length,step,q->note);
    if(authored>span)span=authored;
    if(span>length)span=length;
    uint32_t gate=(uint32_t)(((uint64_t)perf_period(bpm,c->seq_rate)*(span-1u))+
                   ((uint64_t)period*(c->seq_gate?c->seq_gate:75u))/100u);
    if(count==1&&c->harmony.chord){
     uint8_t chord[4],nc=etch_chord_notes(q->note,&c->harmony,chord);
     for(uint8_t ci=0;ci<nc;ci++)perf_on(s,chord[ci],q->velocity,gate);
    }else perf_on(s,q->note,q->velocity,gate);
   }
  }else{
   s->seq_lock_mask=0;
   uint8_t note=c->seq_notes[step%64u];
   if(note<=127)perf_on(s,note,127,s->perf_gate);
  }
  s->perf_step=(uint8_t)((step+1)%length);
 }else {uint8_t note=arp_pick(s,c,keys);if(note<=127)perf_on(s,note,127,s->perf_gate);}
 return 1u; /* step boundary: recalculate oscillator rates this sample */
}
static void envelope(struct etch_native_voice *v,const struct etch_native_controls *c){
 envelope_with_steps(v,c->attack_step,c->decay_step,c->release_step,c->sustain_level);
}
static void native_rates(const struct etch_native_audio *s,const struct etch_native_controls *c,
                         uint32_t increments[8],uint32_t modincrements[8]){
 static const uint8_t ratios[12]={8,1,2,4,12,16,24,32,48,64,96,128};
 for(uint32_t i=0;i<8;i++){
  const struct etch_native_voice *v=&s->voice[i];
  uint32_t factor=v->note>=128&&v->note<2176?(s->bend[(v->note-128)/128]&0x00ffffffu):65536;
  increments[i]=(uint32_t)(((uint64_t)v->inc*factor)>>16);
  const struct etch_native_patch *gp=group_patch_at(etch_native_voice_group((uint8_t)i));
  uint32_t ratio=gp?(gp->mod_ratio<12?gp->mod_ratio:0u):(c->mod_ratio<12?c->mod_ratio:0u);
  uint8_t mode=gp?gp->mod_mode:c->mod_mode;
  uint64_t mod=mode?c->mod_inc:((uint64_t)increments[i]*ratios[ratio])/8;
  modincrements[i]=(uint32_t)(mod>0x70000000u?0x70000000u:mod);
 }
}
static void render_internal(struct etch_native_audio *s,const struct etch_native_controls *c,uint32_t keys,int16_t *out,uint32_t frames,uint8_t stereo){
 if(!s||!c||!out)return;
 /* Null bank is the main-loop GROUPS compilation gate. During a rebuild
  * the audio IRQ stays deterministic and never reads incomplete tables. */
 if(!c->bank && active_groups && (active_groups->group[0].enabled || active_groups->group[1].enabled)){
  for(uint32_t n=0;n<frames*(stereo?2u:1u);n++)out[n]=0;
  return;
 }
 if(c->bank!=s->bank&&!s->bank_fade){s->old_bank=s->bank;s->bank=c->bank;s->bank_fade=256;}
 keys&=0x7ffffffu;
 uint8_t has_groups=(uint8_t)(active_groups &&
    (active_groups->group[0].enabled || active_groups->group[1].enabled));
 uint8_t limit=(c->mono_mode&&!has_groups)?1u:(etch_voice_budget_valid(s->voice_limit)?s->voice_limit:8u);
 uint8_t previous_mode=s->mono_mode;
 s->mono_mode=(uint8_t)(c->mono_mode!=0 && !has_groups);
 if(previous_mode!=s->mono_mode){
  s->mono_count=0u;
  if(s->mono_mode){
   /* Rebuild held panel-key priority on a mode toggle. */
   for(uint8_t k=0;k<27u;k++)if((keys&s->keys)&(1u<<k)){
    int32_t pitch=53+(int32_t)k+12*c->octave;
    if(pitch<0)pitch=0;if(pitch>127)pitch=127;
    mono_push(s,etch_quantize_note((uint8_t)pitch,&c->harmony),k,127u,0u);
   }
  }
 }
 /* Enforce a measured overload budget at a block boundary. Soft-tail a
  * retired voice for 128 samples, without expensive oscillator/filter work.
  * Incoming note-offs remain harmless after the retired voice is released. */
 for(uint32_t i=limit;i<ETCH_NATIVE_VOICES;i++){
  struct etch_native_voice *v=&s->voice[i];
  if(v->active){s->overload_last[i]=v->last;s->overload_tail[i]=128u;
   v->active=v->held=v->down=0;v->gate_left=0;v->last=0;}
 }
 s->harmony=c->harmony;
 struct etch_native_controls perf_control=*c;
 if(c->preview_wave){perf_control.arp_mode=0u;perf_control.seq_run=0u;}
 uint32_t changed=keys^s->keys;
 /* Releases use each voice's original pitch; changing octave cannot strand a key. */
 for(uint32_t k=0;k<27;k++)if(changed&(1u<<k)){
  if(!(keys&(1u<<k))){
   if(s->mono_mode)mono_release(s,(uint16_t)k,0u);
   else for(uint32_t i=0;i<8;i++)if(s->voice[i].note==k)s->voice[i].held=s->voice[i].down=0;
  }
 }
 for(uint32_t k=0;k<27;k++)if((changed&keys)&(1u<<k)&&!perf_control.arp_mode){
  int32_t n=53+(int32_t)k+12*c->octave;if(n<0)n=0;if(n>127)n=127;
  uint8_t tuned=etch_quantize_note((uint8_t)n,&c->harmony);
  if(s->mono_mode)mono_push(s,tuned,(uint16_t)k,127u,0u);
  else {uint32_t slot=on(s,(uint8_t)n,(uint16_t)k);s->voice[slot].inc=etch_note_inc[tuned];}
 }
 perf_order(s,changed,keys);s->keys=keys;
 uint32_t increments[8],modincrements[8],organic_incs[8],organic_modincs[8];
 struct etch_mip_lookup carrier_mip[8],mod_mip[8];
 /* Per-part ADSR coefficients computed once per render block, never per
  * PCM sample. Additional stack only; zero permanent SRAM overhead. */
 /* V1.45: GROUPS config and preset snapshots cannot change within a
  * render callback. Resolve pointers and pre-insert gates just ONCE per
  * block. This eliminates per-voice/per-sample preset lookups and repeated
  * effect-route decoding. All group voices still share the same 8 slots. */
 struct etch_part_cache {
  const struct etch_native_patch *patch;
  const struct etch_preset_dsp *dsp;
  const struct etch_fx_settings *fx;
  const struct etch_mod_matrix *matrix;
  uint8_t preset,pre_active,matrix_active,matrix_needs_wave;
 } parts[4]={0};
 struct etch_fx_settings masked_group_fx[2]; /* block-local; no heap or persistent RAM */
 parts[0].fx=&c->fx_settings;
 parts[0].pre_active=pre_insert_enabled(parts[0].fx);
 for(uint8_t pi=1u;pi<=2u;pi++){
  parts[pi].patch=group_patch_at(pi);
  if(parts[pi].patch){
   parts[pi].preset=group_patch_slot(pi);
   parts[pi].fx=etch_group_fx_for_preset(parts[pi].preset);
   if(parts[pi].fx && (c->fx_bypass_mask & (ETCH_FXB_BIT(ETCH_FXB_A1)|ETCH_FXB_BIT(ETCH_FXB_A2)|ETCH_FXB_BIT(ETCH_FXB_B1)|ETCH_FXB_BIT(ETCH_FXB_B2)))){
    masked_group_fx[pi-1u]=*parts[pi].fx;
    etch_fxb_mask_pre(c->fx_bypass_mask,&masked_group_fx[pi-1u]);
    parts[pi].fx=&masked_group_fx[pi-1u];
   }
   parts[pi].dsp=etch_group_dsp_for_preset(parts[pi].preset);
   parts[pi].pre_active=pre_insert_enabled(parts[pi].fx);
  }
 }
 /* Route definitions and their validity stay fixed throughout one rendered
  * DMA block. Per-note modulation is still computed at full sample rate. */
 for(uint8_t pi=0u;pi<4u;pi++){
  const struct etch_mod_matrix *m=parts[pi].dsp?&parts[pi].dsp->matrix:&c->matrix;
  parts[pi].matrix=m;
  parts[pi].matrix_active=etch_matrix_active(m);
  if(parts[pi].matrix_active)
   for(uint32_t r=0;r<ETCH_MATRIX_ROUTES;r++)
    if(m->route[r].source==3u && m->route[r].amount){parts[pi].matrix_needs_wave=1u;break;}
 }
 struct group_env {uint32_t attack,decay,release;uint16_t sustain;} group_envs[2];
 for(uint8_t gi=0;gi<2u;gi++){
  const struct etch_native_patch *gp=parts[gi+1u].patch;
  group_envs[gi].attack=gp?group_adsr_step(gp->attack_ms):c->attack_step;
  group_envs[gi].decay=gp?group_adsr_step(gp->decay_ms):c->decay_step;
  group_envs[gi].release=gp?group_adsr_step(gp->release_ms):c->release_step;
  group_envs[gi].sustain=gp?gp->sustain:c->sustain_level;
 }
 uint16_t living_from[8]={0},living_to[8]={0};
 native_rates(s,c,increments,modincrements);
 struct etch_native_controls locked=*c;
 uint8_t motion_play=(uint8_t)(c->motion_play&&c->motion&&c->motion->length&&c->motion->mask);
 if(motion_play&&!s->motion_was_playing)s->motion_sample=0u;
 s->motion_was_playing=motion_play;
 uint16_t motion_values[4]={0};
 const struct etch_evolution_settings *e=&c->evolution;
 uint8_t evolving=(uint8_t)(c->mono_mode && etch_evolution_valid(e)&&e->depth!=0u);
 /* When switching MONO -> POLY, stop the old waveshaper immediately.
  * Otherwise its 64-sample release ramp would still consume CPU and
  * color several POLY buffers despite the LFO editor being hidden. */
 if(!c->mono_mode)evolution_smooth=0;
 uint32_t evo_step=evolving?etch_evolution_step(e,(c->clock_follow&&s->clock_bpm)?s->clock_bpm:c->tempo):0u;

 for(uint32_t f=0;f<frames;f++){
  if(s->clock_samples<1000000u)++s->clock_samples;
  if(s->clock_samples>44100u&&s->clock_running){s->clock_running=0;}
  uint8_t rates_changed=perf_tick(s,&perf_control,keys);
  if(rates_changed)native_rates(s,c,increments,modincrements);
  /* V1.76 ORGANIC drift: 32-sample control-rate smoothstep between
   * slow random targets. The old 1/16 target chase settled in ~12ms and
   * held a pitch until the next random target, producing audible steps.
   * `pitch` now stores a segment START, `goal` its END; `time` packs a
   * 16-bit duration and 16-bit elapsed count. No new per-voice RAM.
   * On a live DRIFT=0 change, gently return to neutral in 1.5 seconds.
   * All-zero settings from reset remain an exact, cost-free bypass. */
  if((f&31u)==0u || rates_changed){
   const struct etch_organic_settings *org=&c->organic;
   uint8_t valid=(uint8_t)(c->mono_mode && etch_organic_valid(org));
   for(uint32_t oi=0;oi<ETCH_NATIVE_VOICES;oi++){
    struct etch_native_voice *ov=&s->voice[oi];
    struct organic_voice_state *os=organic_at(oi);
    int32_t deviation=0;
    if(valid && ov->active && (org->drift || org->variance || os->ready)){
     uint16_t duration=(uint16_t)(os->time>>16);
     uint16_t elapsed=(uint16_t)os->time;
     if(org->drift){
      if(os->ready!=1u){
       /* Fresh note, or re-enable after a gentle fade to zero. */
       int32_t start=(os->ready==2u)?
        etch_organic_smooth_pitch(os->pitch,os->goal,elapsed,duration):0;
       os->seed=etch_organic_hash(os->seed);
       os->pitch=start;
       os->goal=etch_organic_target(os->seed,org->drift);
       duration=etch_organic_duration_ticks(org->rate);
       elapsed=0;os->ready=1u;
      }
      if(elapsed>=duration){
       os->pitch=os->goal;
       os->seed=etch_organic_hash(os->seed+0x9e3779b9u);
       os->goal=etch_organic_target(os->seed,org->drift);
       duration=etch_organic_duration_ticks(org->rate);
       elapsed=0;
      }
     }else if(os->ready==1u){
      /* Don't jump back to the original pitch when DRIFT is turned off. */
      os->pitch=etch_organic_smooth_pitch(os->pitch,os->goal,elapsed,duration);
      os->goal=0;
      duration=2068u; /* ~1.5 seconds at 44.1kHz */
      elapsed=0;os->ready=2u;
     }
     if(os->ready==1u || os->ready==2u){
      deviation=etch_organic_smooth_pitch(os->pitch,os->goal,elapsed,duration);
      if(elapsed<duration)elapsed++;
      else if(os->ready==2u){os->ready=3u;os->pitch=os->goal=0;}
      os->time=((uint32_t)duration<<16)|elapsed;
     }
     deviation+=etch_organic_static(ov->birth*2654435761u,org->variance);
    }
    organic_incs[oi]=deviation?etch_organic_pitch_inc(increments[oi],deviation):increments[oi];
    organic_modincs[oi]=deviation?etch_organic_pitch_inc(modincrements[oi],deviation):modincrements[oi];
    carrier_mip[oi]=mip_prepare(organic_incs[oi]);
    mod_mip[oi]=mip_prepare(organic_modincs[oi]);
   }
  }
  /* Per-voice LIVING harmonic envelope: control rate = 32 samples.
   * Weight interpolation across each control tick prevents stepping;
   * note age persists across render blocks without growing the DSP pool. */
  /* In POLY this feature is completely bypassed, including its control
   * loop. Do not do eight weight updates or modulo operations for an
   * effect that is explicitly mono-only. Preset data is left untouched. */
  if((f&31u)==0u){
   for(uint32_t li=0;li<ETCH_NATIVE_VOICES;li++){
    struct etch_native_voice *lv=&s->voice[li];
    uint8_t li_owner=etch_native_voice_group((uint8_t)li);
    const struct etch_preset_dsp *li_dsp=parts[li_owner].dsp;
    const struct etch_living_settings *li_living=li_dsp?&li_dsp->living:&c->living;
    if(lv->active && etch_living_valid(li_living) && li_living->depth){
     uint16_t age=living_voice_age[li];
     living_from[li]=etch_living_weight(age,lv->birth,lv->velocity,li_living);
     living_voice_age[li]=(uint16_t)(((uint32_t)age+1u)%(2u*etch_living_period(li_living->rate)));
     living_to[li]=etch_living_weight(living_voice_age[li],lv->birth,lv->velocity,li_living);
    }else{living_from[li]=0;living_to[li]=0;}
   }
  }
  /* Restore dynamic locks without copying the large control struct 128x. */
  locked.morph=c->morph;locked.depth=c->depth;locked.cutoff=c->cutoff;
  locked.chorus=c->chorus;locked.delay=c->delay;locked.reverb=c->reverb;
  if((f&31u)==0u)locked.fx_settings=c->fx_settings;
  if(motion_play){
   const struct etch_motion_clip *motion=c->motion;
   const uint32_t at=s->motion_sample++;
   if((f&31u)==0u)for(uint32_t k=0;k<4u;k++)if(motion->mask&(1u<<k))motion_values[k]=etch_motion_at(motion,(uint8_t)k,at);
   if(motion->mask&1u)locked.morph=motion_values[0];
   if(motion->mask&2u)locked.depth=motion_values[1];
   if(motion->mask&4u)locked.cutoff=motion_values[2];
   if(motion->mask&8u)locked.reverb=motion_values[3];
   if(s->motion_sample>=(uint32_t)motion->length*ETCH_MOTION_SAMPLES_PER_STEP)s->motion_sample=0u;
  }
  if(s->seq_lock_mask){
   if(s->seq_lock_mask&(1u<<ETCH_LOCK_MORPH))locked.morph=s->seq_locks[ETCH_LOCK_MORPH];
   if(s->seq_lock_mask&(1u<<ETCH_LOCK_DEPTH))locked.depth=s->seq_locks[ETCH_LOCK_DEPTH];
   if(s->seq_lock_mask&(1u<<ETCH_LOCK_CUTOFF))locked.cutoff=s->seq_locks[ETCH_LOCK_CUTOFF];
   if(s->seq_lock_mask&(1u<<ETCH_LOCK_CHORUS))locked.chorus=s->seq_locks[ETCH_LOCK_CHORUS];
   if(s->seq_lock_mask&(1u<<ETCH_LOCK_DELAY))locked.delay=s->seq_locks[ETCH_LOCK_DELAY];
   if(s->seq_lock_mask&(1u<<ETCH_LOCK_REVERB))locked.reverb=s->seq_locks[ETCH_LOCK_REVERB];
  }
  s->morph=slew(s->morph,locked.morph>32767?32767:locked.morph);
  s->depth=slew(s->depth,locked.depth>32767?32767:locked.depth);
  /* Smooth filter controls at sample rate, including automation/locks.
   * Target-independent Q15 state avoids stepped cutoff/resonance changes
   * during held notes. Bypass and dry signal paths remain untouched. */
  filter_cutoff_smooth=slew(filter_cutoff_smooth,locked.cutoff>32767?32767:locked.cutoff);
  filter_resonance_smooth=slew(filter_resonance_smooth,locked.resonance>32767?32767:locked.resonance);
  locked.cutoff=(uint16_t)filter_cutoff_smooth;
  locked.resonance=(uint16_t)filter_resonance_smooth;
  uint32_t evo_phase=evolution_phase;
  if(evolving){
   if(e->sync==2u){uint32_t step=2048u+(uint32_t)e->rate*35000u/100u;
    if(evolution_phase<0xffffffffu-step)evolution_phase+=step;
    else evolution_phase=0xffffffffu;
   }else evolution_phase+=evo_step;
  }
  uint16_t evo_amount=0u;
  if(evolving){
   uint32_t at=e->sync==2u?0u:evo_phase;
   uint32_t tri=(uint32_t)etch_evolution_triangle(at);
   uint32_t goal=(uint32_t)e->depth*32767u/100u;
   /* Off -> zero when EVOLVE is disabled, ON keeps a gentle audible base
    * while the period cycles through stronger transformations. */
   uint32_t amt=e->sync==2u?(uint32_t)(((uint64_t)goal*(evolution_phase>>16))/65535u):
                  (goal*(8192u+(tri*3u/4u)))/32767u;
   if(amt>32767u)amt=32767u;
   evo_amount=(uint16_t)amt;
  }
  /* Parameter motion is click-free: 64-sample ramp into the audible
   * waveshaping amount, with zero work/energy when completely disabled. */
  int32_t dx=(int32_t)evo_amount-evolution_smooth;
  if(dx){int32_t step=dx/64;if(!step)step=dx>0?1:-1;evolution_smooth+=step;}
  evo_amount=(uint16_t)(evolution_smooth<0?0:evolution_smooth>32767?32767:evolution_smooth);
  /* No clicks when entering, exiting or switching the soloed waveform. */
  if(preview_selected!=c->preview_wave){
   if(preview_mix>256u)preview_mix-=256u;
   else{preview_mix=0u;preview_selected=c->preview_wave;}
  }else if(preview_selected && preview_mix<32767u){
   uint32_t next=preview_mix+256u;preview_mix=(uint16_t)(next>32767u?32767u:next);
  }
  /* Compute one smooth old-bank coefficient per PCM frame, not per voice.
   * The 256-frame handoff and per-voice DSP work remain unchanged. */
  uint16_t old_bank_mix=etch_transition_old_mix_q8(s->bank_fade,256u);
  int32_t sum=0,preview_sum=0;uint32_t voices_sounding=0u;
  uint8_t matrix_global_done=0u;uint16_t matrix_reverb=locked.reverb;
  for(uint32_t i=0;i<limit;i++){
   struct etch_native_voice *v=&s->voice[i];if(!v->active)continue;
   uint8_t owner=etch_native_voice_group((uint8_t)i);
   const struct etch_native_patch *group_patch=parts[owner].patch;
   if(c->env_enabled){
    if(group_patch){const struct group_env *ge=&group_envs[owner-1u];
     envelope_with_steps(v,ge->attack,ge->decay,ge->release,ge->sustain);
    }else envelope(v,&locked);
   }
   else if(v->held){v->env+=128;if(v->env>32767)v->env=32767;}else{v->env-=64;if(v->env<0)v->env=0;}
   /* An envelope that reached zero needs no waveform/FX voice calculation. */
   if(!v->env&&!v->held&&!v->fade){v->active=0;continue;}
   if(v->env>256||v->fade)voices_sounding++;
   /* Voice phase offset is applied exactly once at the first sample.
    * Carrier and modulator stay phase-coherent within a voice. */
   struct organic_voice_state *os=organic_at(i);
   if(!os->phase_ready){
    if(c->mono_mode && etch_organic_valid(&c->organic)&&c->organic.phase){
     uint32_t shift=etch_organic_initial_phase(os->seed,c->organic.phase);
     v->p=v->q=shift;
    }
    os->phase_ready=1;
   }
   uint16_t voice_evo=evo_amount;
   uint16_t voice_living=(uint16_t)((int32_t)living_from[i]+
     (((int32_t)living_to[i]-(int32_t)living_from[i])*(int32_t)(f&31u))/32);
   uint16_t voice_morph=(uint16_t)(s->morph<0?0:s->morph>32767?32767:s->morph);
   uint16_t voice_depth=(uint16_t)(s->depth<0?0:s->depth>32767?32767:s->depth);
   /* Multitimbral GROUPS: the three existing bandlimited table banks are
    * assigned main/Group1/Group2 during GROUPS playback. The voice owns
    * its preset even when other MIDI channels are playing concurrently. */
   const struct etch_native_bank *voice_bank=group_patch?&native_banks[owner]:s->bank;
   uint8_t gslot=parts[owner].preset;
   const struct etch_fx_settings *voice_fx=group_patch?parts[owner].fx:&c->fx_settings;
   if(group_patch){
    voice_morph=group_patch->morph>32767u?32767u:group_patch->morph;
    voice_depth=group_patch->depth>32767u?32767u:group_patch->depth;
   }
   /* Channel pressure adds up to 12.5% morph, per channel/voice. Zero
    * pressure is bit-exact with V1.40 Phase 5; 16-bit bounded arithmetic.
    * This path does not alter stored patch records (PJT28 compatible). */
   if(v->note>=128u){
    uint32_t midi_ch=((uint32_t)v->note-128u)/128u;
    if(midi_ch<16u){
     uint32_t added=(uint32_t)(s->bend[midi_ch]>>24)*4096u/127u;
     uint32_t shaped=(uint32_t)voice_morph+added;
     voice_morph=(uint16_t)(shaped>32767u?32767u:shaped);
    }
   }
   const struct etch_preset_dsp *voice_dsp=parts[owner].dsp;
   uint16_t voice_cutoff=group_patch?group_patch->cutoff:locked.cutoff;
   /* Fast bypass for legacy projects; matrix route computation is per voice.
    * Wave C is sampled at the actual modulator phase for that note. */
   const struct etch_mod_matrix *voice_matrix=parts[owner].matrix;
   if(parts[owner].matrix_active){
    int16_t matrix_wave=parts[owner].matrix_needs_wave?
      (int16_t)readwave_prepared(voice_bank,2,v->q,&mod_mip[i]):0;
    /* MIDI identities are 128 + channel*128 + pitch. Panel notes use
     * identity == pitch. Channel pressure is retained in bend high byte. */
    uint8_t matrix_pressure=0u;
    uint8_t matrix_note=(uint8_t)(v->note&127u);
    if(v->note>=128u && v->note<2176u){
     uint32_t ch=((uint32_t)v->note-128u)/128u;
     matrix_pressure=(uint8_t)(s->bend[ch]>>24);
    }
    uint16_t *space_output=(!group_patch && !matrix_global_done)?&matrix_reverb:0;
    etch_matrix_apply_all_active_ex(voice_matrix,&voice_morph,&voice_depth,
      &voice_cutoff,space_output,v->velocity,(uint16_t)v->env,matrix_wave,
      voice_living,matrix_pressure,matrix_note);
    if(space_output)matrix_global_done=1u;
   }

   uint8_t amp_mode=group_patch?(voice_dsp?etch_wamp_mode(voice_dsp->spectral_wave):0u):c->wave_amp_mode;
   uint16_t amp_depth=group_patch?(voice_dsp?etch_wamp_q15(etch_wamp_depth6(voice_dsp->velocity_curve)):0u):c->wave_amp_depth;
   uint8_t amp_enabled=(uint8_t)(amp_mode && amp_depth);
   int32_t amp_wave=0;
   int32_t sample=preview_mix==32767u?0:
    carrier(v,voice_fx,voice_bank,v->p,v->q,organic_incs[i],organic_modincs[i],voice_morph,voice_depth,group_patch?group_patch->mod_target:c->mod_target,voice_evo,evolving?e->mode:0u,&carrier_mip[i],&mod_mip[i],
            parts[group_patch?owner:0u].pre_active,amp_mode,amp_enabled?&amp_wave:0);
   int32_t solo=preview_mix?preview_carrier(v,c,s->bank,v->p,v->q,organic_incs[i],organic_modincs[i],preview_selected,&carrier_mip[i],&mod_mip[i]):0;
   if(s->bank_fade && preview_mix && s->old_bank){
    int32_t older=preview_carrier(v,c,s->old_bank,v->p,v->q,organic_incs[i],organic_modincs[i],preview_selected,&carrier_mip[i],&mod_mip[i]);
    solo+=(int32_t)(((int64_t)(older-solo)*old_bank_mix)/256);
   }
   if(s->bank_fade && !group_patch && preview_mix<32767u){
    int32_t old=carrier(v,0,s->old_bank,v->p,v->q,organic_incs[i],organic_modincs[i],voice_morph,voice_depth,c->mod_target,voice_evo,evolving?e->mode:0u,&carrier_mip[i],&mod_mip[i],0u,0u,0);
    sample+=(int32_t)(((int64_t)(old-sample)*old_bank_mix)/256);
   }
   /* One LIVING pass per active voice. V1.31 applied the fundamental
    * reconstruction in every carrier() invocation, including old-bank
    * crossfades. At 8 voices that could overrun the ALNK DMA window. */
   const struct etch_living_settings *voice_living_cfg=voice_dsp?&voice_dsp->living:&c->living;
   if(preview_mix!=32767u && voice_living_cfg->depth){
    sample=living_shape(sample,&os->living_lp,organic_incs[i],voice_living,
                        voice_living_cfg->color,1u);
   }
   /* Each voice gets its own cutoff/resonance/filter type. Restore the
    * shared working controls immediately after filtering this voice. */
   uint16_t saved_cutoff=locked.cutoff,saved_resonance=locked.resonance;
   uint8_t saved_filter_type=locked.filter_type;
   if(group_patch){
    locked.cutoff=voice_cutoff;locked.resonance=group_patch->resonance;
    locked.filter_type=group_patch->filter_type;
   }else if(parts[owner].matrix_active)locked.cutoff=voice_cutoff;
   sample=filter(v,&locked,sample);
   if(has_groups){uint16_t voice_drive=(c->fx_bypass_mask & ETCH_FXB_BIT(ETCH_FXB_DRIVE))?0u:(group_patch?group_patch->drive:c->drive);
    if(voice_drive)sample=etch_drive_softclip(sample,voice_drive);
   }
   locked.cutoff=saved_cutoff;locked.resonance=saved_resonance;
   locked.filter_type=saved_filter_type;
   uint8_t velocity_curve=group_patch?etch_group_velocity_for_preset(gslot):c->velocity_curve;
   uint8_t expressive_velocity=etch_expressive_velocity(v->velocity,velocity_curve);
   /* V1.84 per-voice audio-rate amplitude sculpt. ADSR remains independent;
    * OFF/zero-depth keeps the legacy PCM bit-for-bit identical. */
   /* Solo preview has its own carrier path; do not apply a stale gain from
    * a carrier intentionally skipped during a full-wave audition. */
   if(amp_enabled && preview_mix!=32767u){
    sample=etch_wamp_apply(sample,amp_wave,amp_depth);
    solo=etch_wamp_apply(solo,amp_wave,amp_depth);
   }
   sample=(sample*v->env)/32768;sample=(sample*expressive_velocity)/127;
   solo=(solo*v->env)/32768;solo=(solo*expressive_velocity)/127;
   if(v->attack_fade){
    /* Cubic zero-slope fade-in reduces click energy at note-on (and hard
     * retrigger) without adding a buffer, delay, or a 64-bit multiply. */
    solo=etch_attack_apply(solo,v->attack_fade);
    sample=etch_attack_apply(sample,v->attack_fade);
    --v->attack_fade;
   }
   if(v->fade){
    uint16_t old_tail_mix=etch_transition_old_mix_q8(v->fade,ETCH_TRANSITION_SAMPLES);
    sample+=(int32_t)(((int64_t)v->tail*old_tail_mix)/256);
    --v->fade;
   }
   v->last=preview_mix?solo:sample;sum+=sample;preview_sum+=solo;v->p+=organic_incs[i];v->q+=organic_modincs[i];
   if(!v->env&&!v->held&&!v->fade)v->active=0;
  }
  for(uint32_t i=limit;i<ETCH_NATIVE_VOICES;i++)if(s->overload_tail[i]){
   /* A brief bounded fade instead of abruptly cutting a stolen held note. */
   sum+=(s->overload_last[i]*(int32_t)s->overload_tail[i])/128;
   --s->overload_tail[i];
  }
  /* Q16 master gain slews once PER AUDIO SAMPLE, not once per ADC read or
   * 128-frame block. ~6 ms smoothing at 44.1 kHz suppresses zipper noise.
   * An 8/16-bit knob/ADC step must never become a PCM amplitude step.
   * The accumulator belongs to audio_core and survives successive blocks. */
  const int32_t gain_target=(int32_t)(c->gain>32767?32767:c->gain)*65536;
  int32_t gain_delta=gain_target-s->master_gain_q16;
  if(gain_delta){
   int32_t gain_step=gain_delta/256;
   if(!gain_step)gain_step=gain_delta>0?1:-1;
   s->master_gain_q16+=gain_step;
  }
  uint32_t gain=(uint32_t)(s->master_gain_q16>>16);
  if(s->bank_fade&&!--s->bank_fade)s->old_bank=0;
  /* V1.16 extends V1.10: V1.09 used a fixed /4 mixer even for 8 loud notes, so a
   * full-amplitude chord could hard-clip before reaching the channel EQ.
   * Keep 1..4-voice loudness unchanged; progressively reserve headroom
   * for 5..8 overlapping voices, with slew-limited transitions. No jump
   * in loudness when notes are allocated/released during ARP playback. */
  /* V1.16: use admission-aware headroom while new notes are in their
   * attack ramp. Before this change, a 4->8-note chord could wait almost
   * 3ms for enough headroom and hard-clip into the FX input. Headroom
   * increases within 32 samples, decreases over 1024, and is continuous
   * at each sample. The instantaneous safety knee below is a second line
   * of defence when filter resonance or coherent phases exceed nominal. */
  uint32_t admitted=voices_sounding;
  for(uint32_t i=0;i<limit;i++)if(s->voice[i].active &&
      s->voice[i].held && s->voice[i].attack_fade &&
      s->voice[i].env<=256)++admitted;
  s->poly_div_q10=etch_poly_headroom_step(s->poly_div_q10,admitted);
  /* 8 voices with max Q15 carriers and a 128-sample soft tail are below
   * +/-524288. Avoid two emulated 64-bit VARIABLE divisions every 44.1kHz
   * sample. The bound also protects against unexpected transients. */
  int32_t bounded_sum=sum>524288?524288:sum< -524288?-524288:sum;
  int32_t mix=(bounded_sum*1024)/s->poly_div_q10;
  mix=etch_poly_safe_sample(mix);
  int32_t preview_dry=0;
  if(preview_mix){
   int32_t bounded_preview=preview_sum>524288?524288:preview_sum< -524288?-524288:preview_sum;
   preview_dry=etch_poly_safe_sample((bounded_preview*1024)/s->poly_div_q10);
  }
  /* Ramp DRIVE once per sample. The original version switched gain and
   * waveshaping abruptly at buffer boundaries, generating zipper clicks.
   * Keep the exact dry path when both target and ramp are zero. */
  {
   int32_t target=c->drive>32767?32767:(int32_t)c->drive;
   int32_t delta=target-drive_smooth;
   if(delta){int32_t step=delta/256;if(!step)step=delta>0?1:-1;drive_smooth+=step;}
   if(drive_smooth && !has_groups){
    /* No hard clamp before nonlinear processing: gradual compression
     * avoids flat-topped transient distortion and stays bounded. */
    mix=etch_drive_softclip(mix,(uint32_t)drive_smooth);
   }
  }

  /* Global C modulation is sampled from a currently held voice. It never
   * changes stored parameters, only the live copy for this audio sample. */
  if((f&31u)==0u && etch_c_fx_target(&c->fx_settings) && etch_c_fx_depth(&c->fx_settings)){
   int32_t modulation=0;
   for(uint32_t vi=0;vi<limit;vi++)if(s->voice[vi].active){
    modulation=readwave(s->bank,2,s->voice[vi].q,modincrements[vi]);break;
   }
   const int32_t desired=(int32_t)(((int64_t)modulation*etch_c_fx_depth(&c->fx_settings))/131072);
   s->c_fx_smooth+=(desired-s->c_fx_smooth)/4;
   uint16_t *destination=0;
   switch(etch_c_fx_target(&c->fx_settings)){
    case 1:destination=&locked.fx_settings.phaser_rate;break;
    case 2:destination=&locked.fx_settings.decim_bits;break;
    case 3:destination=&locked.fx_settings.delay_time;break;
    case 4:destination=&locked.fx_settings.flanger_rate;break;
    case 5:destination=&locked.fx_settings.trem_depth;break;
    case 6:destination=&locked.fx_settings.eq_mid;break;
    case 7:destination=&locked.fx_settings.reverb_size;break;
    case 8:destination=&locked.fx_settings.reverb_decay;break;
    case 9:destination=&locked.fx_settings.chorus_rate;break;
    case 10:destination=&locked.fx_settings.chorus_depth;break;
    case 11:destination=&locked.fx_settings.delay_feedback;break;
    case 12:destination=&locked.fx_settings.ring_rate;break;
    case 13:destination=&locked.fx_settings.eq_low;break;
    case 14:destination=&locked.fx_settings.eq_high;break;
    case 15:destination=&locked.fx_settings.comp_threshold;break;
    case 16:destination=&locked.fx_settings.comp_ratio;break;
    case 17:destination=&locked.fx_settings.trem_rate;break;
    case 18:destination=&locked.fx_settings.phaser_depth;break;
    case 19:destination=&locked.fx_settings.flanger_depth;break;
    case 20:destination=&locked.fx_settings.decim_rate;break;
    case 21:destination=&locked.fx_settings.folder_fold;break;
    case 22:destination=&locked.fx_settings.tape_wow;break;
    case 23:destination=&locked.fx_settings.tape_flutter;break;
    case 24:destination=&locked.fx_settings.reverse_time;break;
    case 25:destination=&locked.fx_settings.vibrato_rate;break;
    case 26:destination=&locked.fx_settings.rotary_rate;break;
    case 27:destination=&locked.fx_settings.wah_sens;break;
    case 28:destination=&locked.reverb;break;
    case 29:destination=&locked.delay;break;
    case 30:destination=&locked.chorus;break;
    case 31:destination=&locked.fx_settings.eq_hpf;break;
   }
   if(destination){int32_t n=(int32_t)*destination+s->c_fx_smooth;
    *destination=(uint16_t)(n<0?0:n>32767?32767:n);}
  }else if((f&31u)==0u)s->c_fx_smooth=0;
  if(matrix_global_done)locked.reverb=matrix_reverb;
  etch_fxb_mask_master(c->fx_bypass_mask,&locked.fx_settings,&locked.chorus,&locked.delay,&locked.reverb);
  int16_t l=0,r=0;
  etch_fx_process_stereo_masked(&s->fx,(int16_t)clamp15(mix),locked.chorus,locked.delay,locked.reverb,&locked.fx_settings,c->fx_bypass_mask,&l,&r);
  if(preview_mix){
   int32_t alpha=preview_mix;
   l=(int16_t)clamp15(((int32_t)l*(32767-alpha)+preview_dry*alpha)/32767);
   r=(int16_t)clamp15(((int32_t)r*(32767-alpha)+preview_dry*alpha)/32767);
  }
  /* The effects may run out a genuine delay/reverb tail after note-off,
   * but after 3 s of voice-free silence they MUST NOT produce perpetual
   * low-frequency output. Smooth the gate in 256 samples (no hard click).
   * Reopening on a new note is also slewed, preserving normal ADSR. */
  if(voices_sounding || sum || s->overload_tail[0] || s->overload_tail[1]){
   s->idle_samples=0u;
  }else if(s->idle_samples<132300u)++s->idle_samples;
  int32_t gate_target=s->idle_samples>=132300u?0:32767;
  if(s->idle_gain<gate_target){s->idle_gain+=128;if(s->idle_gain>gate_target)s->idle_gain=gate_target;}
  else if(s->idle_gain>gate_target){s->idle_gain-=128;if(s->idle_gain<gate_target)s->idle_gain=gate_target;}
  /* Both controls are Q15: combine them ONCE using a 32-bit multiply.
   * The previous candidate used two signed 64-bit divisions PER SAMPLE,
   * which would jeopardize the FM1 realtime budget under heavy FX. */
  uint32_t effective_gain=(uint32_t)((gain*(uint32_t)s->idle_gain+16383u)/32767u);
  l=(int16_t)clamp15(((int32_t)l*(int32_t)effective_gain)/32768);
  r=(int16_t)clamp15(((int32_t)r*(int32_t)effective_gain)/32768);
  if(stereo){out[f*2]=l;out[f*2+1]=r;}else out[f]=l;
 }
}

void etch_native_render(struct etch_native_audio *s,const struct etch_native_controls *c,uint32_t keys,int16_t *out,uint32_t frames){
 render_internal(s,c,keys,out,frames,0);
}
void etch_native_render_stereo(struct etch_native_audio *s,const struct etch_native_controls *c,uint32_t keys,int16_t *out,uint32_t frames){
 render_internal(s,c,keys,out,frames,1);
}

int16_t etch_native_bank_preview(const struct etch_native_bank *bank,uint16_t morph,uint16_t depth,uint32_t phase){
 if(morph>32767)morph=32767;if(depth>32767)depth=32767;
 int32_t mod=readwave(bank,2,phase,1000000u);
 phase+=((uint32_t)mod*(uint32_t)depth*2u);
 int32_t a=readwave(bank,0,phase,1000000u),b=readwave(bank,1,phase,1000000u);
 return (int16_t)(a+(int32_t)(((int32_t)(b-a)*(int32_t)morph)/32768));
}

int16_t etch_native_preview(uint16_t morph,uint16_t depth,uint32_t phase){return etch_native_bank_preview(0,morph,depth,phase);}

void etch_native_filter_ramp_state(int32_t *cutoff,int32_t *resonance){
 if(cutoff)*cutoff=filter_cutoff_smooth;
 if(resonance)*resonance=filter_resonance_smooth;
}
