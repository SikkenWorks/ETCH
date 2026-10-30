/* SPDX-License-Identifier: GPL-3.0-only
 * Main-thread-only harmonic reconstruction. Never called by audio IRQ.
 */
#include "wave_editor.h"
#include "native_tables.h"
#include "native_sculpt.h"
static int32_t bounded(int32_t n,int32_t lo,int32_t hi){return n<lo?lo:n>hi?hi:n;}
void etch_editor_init(struct etch_wave_editor *e){
 for(uint32_t w=0;w<3;w++)for(uint32_t i=0;i<128;i++)e->points[w][i]=etch_tables[w][0][i*4];
 e->wave=e->cursor=e->has_undo=e->undo_count=e->undo_head=e->redo_count=0;
 for(uint8_t w=0;w<3;w++){struct etch_wave_fx_params zero={0};e->fx[w]=zero;e->fx[w].mix=32767;}e->dirty=1;e->revision=1;
}
void etch_editor_select(struct etch_wave_editor *e,int32_t delta){int32_t n=(e->wave+delta)%3;if(n<0)n+=3;e->wave=(uint8_t)n;}
void etch_editor_move(struct etch_wave_editor *e,int32_t left,int32_t right,int32_t up,int32_t down){
 /* Directional knobs: either rotary direction counts as its displayed arrow. */
 left=bounded(left,-127,127);right=bounded(right,-127,127);
 up=bounded(up,-64,64);down=bounded(down,-64,64);
 int32_t x=e->cursor+(right<0?-right:right)-(left<0?-left:left);e->cursor=(uint8_t)bounded(x,0,127);
 int32_t dy=((up<0?-up:up)-(down<0?-down:down))*512;
 if(!dy)return;
 int16_t next=(int16_t)bounded((int32_t)e->points[e->wave][e->cursor]+dy,-32768,32767);
 if(next==e->points[e->wave][e->cursor])return;
 etch_editor_checkpoint(e);
 e->has_undo=1;e->points[e->wave][e->cursor]=next;e->dirty=1;++e->revision;
}
/* Undo and redo are a single reversible circular deque of 16 snapshots.
 * This is called only from the main UI thread, never the audio ISR.
 * Invariant: 0 <= undo_count + redo_count <= 16.
 * Every swap exchanges all 3 x 128 exact signed 16-bit control points.
 * Undo/redo retain all precision, and the 16-step user-facing depth is kept. */
_Static_assert(sizeof(((struct etch_wave_editor*)0)->history) == 16u*3u*128u*sizeof(int16_t),
               "16 complete reversible wave snapshots required");
static void etch_editor_swap_snapshot(struct etch_wave_editor *e,uint8_t index){
 for(uint32_t w=0;w<3u;w++)for(uint32_t i=0;i<128u;i++){
  int16_t tmp=e->points[w][i];
  e->points[w][i]=e->history[index][w][i];
  e->history[index][w][i]=tmp;
 }
}
void etch_editor_checkpoint(struct etch_wave_editor *e){
 for(uint32_t w=0;w<3u;w++)for(uint32_t i=0;i<128u;i++)
  e->history[e->undo_head][w][i]=e->points[w][i];
 e->undo_head=(uint8_t)((e->undo_head+1u)&15u);
 if(e->undo_count<16u)++e->undo_count;
 e->has_undo=1u;
 e->redo_count=0u; /* new edit invalidates redo history */
}
void etch_editor_undo(struct etch_wave_editor *e){
 if(!e->undo_count)return;
 e->undo_head=(uint8_t)((e->undo_head+15u)&15u);
 etch_editor_swap_snapshot(e,e->undo_head);
 --e->undo_count;
 ++e->redo_count;
 e->has_undo=(uint8_t)(e->undo_count!=0u);
 e->dirty=1u;++e->revision;
}
void etch_editor_redo(struct etch_wave_editor *e){
 if(!e->redo_count)return;
 etch_editor_swap_snapshot(e,e->undo_head);
 e->undo_head=(uint8_t)((e->undo_head+1u)&15u);
 ++e->undo_count;
 --e->redo_count;
 e->has_undo=1u;
 e->dirty=1u;++e->revision;
}
void etch_editor_compile_source(const int16_t points[3][128], const struct etch_wave_fx_params fx[3],struct etch_native_bank *out){
 static const uint8_t harmonics[6]={64,32,16,8,2,1};
 int32_t cosine[65],sine[65],scratch[512];
 int16_t shaped[128];
 /* The sine reference has Q15 amplitude 29000; divide by that exact scale. */
 for(uint32_t w=0;w<3;w++){
  etch_wave_fx_apply(points[w],&fx[w],shaped);
  for(uint32_t h=1;h<=64;h++){
   int64_t c=0,s=0;
   for(uint32_t n=0;n<128;n++){
    uint32_t phase=(h*n*4)&511u;
    s+=(int64_t)shaped[n]*etch_tables[0][0][phase];
    c+=(int64_t)shaped[n]*etch_tables[0][0][(phase+128)&511u];
   }
   sine[h]=(int32_t)(s/(64*29000));cosine[h]=(int32_t)(c/(64*29000));
   if(h==64){sine[h]=0;cosine[h]/=2;} /* real-input Nyquist coefficient */
  }
  int32_t peak=1;
  for(uint32_t l=0;l<6;l++){
   for(uint32_t n=0;n<512;n++){
    int64_t v=0;
    for(uint32_t h=1;h<=harmonics[l];h++){
     uint32_t phase=(h*n)&511u;
     v+=(int64_t)sine[h]*etch_tables[0][0][phase]+(int64_t)cosine[h]*etch_tables[0][0][(phase+128)&511u];
    }
    scratch[n]=(int32_t)(v/29000);
    int32_t a=scratch[n]<0?-scratch[n]:scratch[n];if(l==0&&a>peak)peak=a;
   }
   for(uint32_t n=0;n<512;n++)out->table[w][l][n]=(int16_t)bounded((int32_t)((int64_t)scratch[n]*29000/peak),-29000,29000);
  }
 }
}

void etch_editor_compile(const struct etch_wave_editor *e,struct etch_native_bank *out){
 etch_editor_compile_source(e->points,e->fx,out);
}

/* Tools act on editable points, checkpoint exactly once and rebuild outside IRQ. */
void etch_editor_tool(struct etch_wave_editor *e,uint8_t tool,int32_t parameter){
 if(!e||tool>11)return;
 int16_t tmp[128];int16_t *wave=e->points[e->wave];
 for(uint32_t i=0;i<128;i++)tmp[i]=wave[i];
 int32_t pos=bounded(e->cursor,0,127),end=bounded(pos+parameter,0,127);
 if(tool==0){
  int32_t lo=pos<end?pos:end,hi=pos>end?pos:end;
  if(lo==hi)return;
  for(int32_t i=lo;i<=hi;i++)tmp[i]=(int16_t)(wave[lo]+((int64_t)(wave[hi]-wave[lo])*(i-lo))/(hi-lo));
 }else if(tool==1){for(uint32_t i=0;i<128;i++)tmp[i]=(int16_t)((wave[(i+127)&127]+(int32_t)wave[i]*2+wave[(i+1)&127])/4);}
 else if(tool==2){for(uint32_t i=64;i<128;i++)tmp[i]=wave[127-i];}
 else if(tool==3){for(uint32_t i=0;i<128;i++)tmp[i]=(int16_t)(wave[i]==-32768?32767:-wave[i]);}
 else if(tool==4){
  int64_t sum=0;for(uint32_t i=0;i<128;i++)sum+=wave[i];int32_t mean=(int32_t)(sum/128),peak=1;
  for(uint32_t i=0;i<128;i++){int32_t a=wave[i]-mean;if(a<0)a=-a;if(a>peak)peak=a;}
  for(uint32_t i=0;i<128;i++)tmp[i]=(int16_t)bounded((int32_t)(((int64_t)(wave[i]-mean)*29000)/peak),-29000,29000);
 }else if(tool==5){for(uint32_t i=0;i<128;i++)tmp[i]=0;}
 else if(tool==6){uint32_t seed=0x45544348u^e->revision;for(uint32_t i=0;i<128;i++){seed=seed*1664525u+1013904223u;tmp[i]=(int16_t)((int32_t)(seed>>16)-32768);}}
 else if(tool==7){for(uint32_t i=0;i<128;i++)tmp[i]=etch_tables[0][0][4*i];}
 else if(tool==8){for(uint32_t i=0;i<128;i++)tmp[i]=(int16_t)(i<64?(int32_t)i*906-29000:29000-(int32_t)(i-64)*906);}
 else if(tool==9){for(uint32_t i=0;i<128;i++)tmp[i]=(int16_t)((int32_t)i*453-29000);}
 else if(tool==10){for(uint32_t i=0;i<128;i++)tmp[i]=i<64?29000:-29000;}
 else if(tool==11){int32_t d=parameter%128;if(d<0)d+=128;for(uint32_t i=0;i<128;i++)tmp[i]=wave[(i+d)&127];}
 uint8_t different=0;for(uint32_t i=0;i<128;i++)if(tmp[i]!=wave[i]){different=1;break;}if(!different)return;
 etch_editor_checkpoint(e);for(uint32_t i=0;i<128;i++)wave[i]=tmp[i];
 e->dirty=1;++e->revision;
}
/* Explicit destructive flattening. The host keeps FX nondestructive until BAKE. */
void etch_editor_bake_fx(struct etch_wave_editor *e){
 if(!e)return;
 const uint8_t w=e->wave;
 int16_t points[128];etch_wave_fx_apply(e->points[w],&e->fx[w],points);
 etch_editor_checkpoint(e);
 for(uint32_t i=0;i<128;i++)e->points[w][i]=points[i];
 struct etch_wave_fx_params neutral={0};neutral.mix=32767;e->fx[w]=neutral;
 e->dirty=1;++e->revision;
}

/* K1 counterclockwise/clockwise = LEFT/RIGHT. K2 = DOWN/UP.
 * Preserve 128-sample patch format and all synthesis resolution while showing
 * only 32 manipulable anchors. Only 1 undo checkpoint per edit command. */
void etch_editor_move_xy32(struct etch_wave_editor *e,int32_t xturns,int32_t yturns){
 if(!e)return;
 int32_t grip=(int32_t)e->cursor/4;
 grip=bounded(grip+bounded(xturns,-32,32),0,31);
 e->cursor=(uint8_t)(grip*4);
 if(!yturns)return;
 int32_t current=e->points[e->wave][e->cursor];
 /* Stronger, still bounded: one detent sculpts a useful part of the cycle.
  * The original narrow 7-tap / 1024-step brush barely changed timbre. */
 int32_t edited=bounded(current+bounded(yturns,-32,32)*2560,-32768,32767);
 if(edited==current)return;
 etch_editor_checkpoint(e);
 int32_t diff=edited-current;
 /* A 17-point weighted brush produces an audible contour change while
  * keeping adjacent points continuous. Wrap at the single-cycle boundary. */
 for(int32_t offset=-8;offset<=8;offset++){
  uint32_t ix=(uint32_t)((e->cursor+offset+128)%128);
  int32_t level=(int32_t)e->points[e->wave][ix]+diff*(9-(offset<0?-offset:offset))/9;
  e->points[e->wave][ix]=(int16_t)bounded(level,-32768,32767);
 }
 e->dirty=1;++e->revision;
}

/* K3 chooses partial 2..16; K4 adds or subtracts harmonic energy from the
 * actual 128-point wave (NOT a graphical overlay and not a separate FX send).
 * Subsequent recompile regenerates all six band-limited mipmaps outside ISR.
 * Keeping the change in points makes existing per-wave SAVE and undo work.
 */
void etch_editor_add_partial(struct etch_wave_editor *e,uint8_t harmonic,int32_t turns){
 if(!e||!turns||harmonic<2u||harmonic>16u)return;
 int32_t gain=bounded(turns,-32,32)*3072;
 etch_editor_checkpoint(e);
 for(uint32_t i=0;i<128;i++){
  uint32_t phase=(uint32_t)harmonic*i*4u&511u;
  int32_t part=etch_tables[0][0][phase];
  int32_t next=(int32_t)e->points[e->wave][i]+(int32_t)(((int64_t)part*gain)/29000);
  e->points[e->wave][i]=(int16_t)bounded(next,-32768,32767);
 }
 e->dirty=1u;++e->revision;
}

/* Wave Genetics, in the MAIN THREAD. Both parent waves remain untouched.
 * A coherent multi-point crossover envelope gives recognizable inheritance,
 * with bounded low-passed mutation rather than uncorrelated white noise.
 * The existing undo stack is checked BEFORE overwriting C. */
void etch_editor_genetics(struct etch_wave_editor *e,uint8_t mix,uint8_t mutate,uint8_t seed){
 if(!e)return;
 if(mix>100u)mix=100u;if(mutate>100u)mutate=100u;
 int16_t child[128];
 uint32_t rng=0x45544348u^((uint32_t)seed*2654435761u);
 int32_t noise=0;
 for(uint32_t i=0;i<128u;i++){
  /* Mix defines overall balance, a slow crossover adds shape variation. */
  rng=rng*1664525u+1013904223u;
  int32_t shift=((int32_t)((rng>>24)&255u)-128)*(int32_t)mutate/100;
  noise=(noise*7+shift*120)/8;
  /* A/B phase alignment is inherited from the user's original drawings. */
  int32_t ab=((int32_t)e->points[0][i]*(100-(int32_t)mix)+
                (int32_t)e->points[1][i]*mix)/100;
  /* Modest correlated perturbation; result never blows up on hard corners. */
  child[i]=(int16_t)bounded(ab+noise,-30000,30000);
 }
 uint8_t changed=0u;
 for(uint32_t i=0;i<128u;i++)if(e->points[2][i]!=child[i]){changed=1u;break;}
 if(!changed)return;
 etch_editor_checkpoint(e);
 for(uint32_t i=0;i<128u;i++)e->points[2][i]=child[i];
 e->wave=2u;e->dirty=1u;++e->revision;
}

/* V1.24: actual harmonic-domain editing, not a graphic-only spectral EQ.
 * Analyse one 128-point periodic wave against the same 512-point sine basis
 * used by the production wavetable compiler. Preserve phase, modify only
 * the chosen partial's complex coefficient, then bake into editable points.
 * Time-critical audio always reads precompiled antialiased mipmap tables.
 */
static void spectral_coeff(const int16_t *points,uint32_t h,int32_t *si,int32_t *co){
 int64_t s=0,c=0;
 for(uint32_t i=0;i<128u;i++){
  uint32_t p=(h*i*4u)&511u;
  s+=(int64_t)points[i]*etch_tables[0][0][p];
  c+=(int64_t)points[i]*etch_tables[0][0][(p+128u)&511u];
 }
 *si=(int32_t)(s/(64LL*29000LL));*co=(int32_t)(c/(64LL*29000LL));
}
static void spectral_commit(struct etch_wave_editor *e,const int32_t *newpoints){
 int32_t max=1;uint8_t any=0;
 for(uint32_t i=0;i<128u;i++){
  int32_t a=newpoints[i]<0?-newpoints[i]:newpoints[i];
  if(a>max)max=a;
 }
 int16_t out[128];
 for(uint32_t i=0;i<128u;i++){
  int32_t value=max>30000?(int32_t)((int64_t)newpoints[i]*30000/max):newpoints[i];
  out[i]=(int16_t)bounded(value,-30000,30000);
  if(out[i]!=e->points[e->wave][i])any=1u;
 }
 if(!any)return;
 etch_editor_checkpoint(e);
 for(uint32_t i=0;i<128u;i++)e->points[e->wave][i]=out[i];
 e->dirty=1u;++e->revision;
}
void etch_editor_spectral_partial(struct etch_wave_editor *e,uint8_t harmonic,int32_t turns){
 if(!e||!turns||harmonic<1u||harmonic>32u)return;
 int32_t si=0,co=0;turns=bounded(turns,-20,20);
 spectral_coeff(e->points[e->wave],harmonic,&si,&co);
 /* +/-20% of existing partial per detent; a missing partial is deliberately
  * created at a reliably audible, bounded starting level. */
 int32_t delta_s=(int32_t)((int64_t)si*turns*20/100);
 int32_t delta_c=(int32_t)((int64_t)co*turns*20/100);
 if(!delta_s&&!delta_c){delta_s=turns*1200;}
 int32_t points[128];
 for(uint32_t i=0;i<128u;i++){
  uint32_t p=((uint32_t)harmonic*i*4u)&511u;
  int32_t delta=(int32_t)(((int64_t)delta_s*etch_tables[0][0][p]+(int64_t)delta_c*etch_tables[0][0][(p+128u)&511u])/29000);
  points[i]=(int32_t)e->points[e->wave][i]+delta;
 }
 spectral_commit(e,points);
}
void etch_editor_spectral_tilt(struct etch_wave_editor *e,int32_t turns){
 if(!e||!turns)return;
 turns=bounded(turns,-16,16);
 int32_t s[65]={0},c[65]={0};
 for(uint32_t h=2u;h<=64u;h++)spectral_coeff(e->points[e->wave],h,&s[h],&c[h]);
 int32_t points[128];
 for(uint32_t i=0;i<128u;i++){
  int64_t delta=0;
  for(uint32_t h=2u;h<=64u;h++){
   /* 10%/detent at h=64, tapering smoothly to zero at fundamental. */
   int32_t ds=(int32_t)((int64_t)s[h]*turns*10*(int32_t)(h-1u)/(100*63));
   int32_t dc=(int32_t)((int64_t)c[h]*turns*10*(int32_t)(h-1u)/(100*63));
   uint32_t p=(h*i*4u)&511u;
   delta+=(int64_t)ds*etch_tables[0][0][p]+(int64_t)dc*etch_tables[0][0][(p+128u)&511u];
  }
  points[i]=(int32_t)e->points[e->wave][i]+(int32_t)(delta/29000);
 }
 spectral_commit(e,points);
}
uint16_t etch_editor_spectral_level(const struct etch_wave_editor *e,uint8_t harmonic){
 if(!e||harmonic<1u||harmonic>32u)return 0;
 int32_t s=0,c=0;spectral_coeff(e->points[e->wave],harmonic,&s,&c);
 uint32_t approx=(uint32_t)((s<0?-s:s)+(c<0?-c:c));
 return (uint16_t)(approx>=29000u?100u:approx*100u/29000u);
}
uint16_t etch_editor_spectral_brightness(const struct etch_wave_editor *e){
 if(!e)return 0;
 uint32_t weighted=0,total=0;
 for(uint32_t h=1u;h<=32u;h++){
  int32_t s=0,c=0;spectral_coeff(e->points[e->wave],h,&s,&c);
  uint32_t amplitude=(uint32_t)((s<0?-s:s)+(c<0?-c:c));
  weighted+=amplitude*(h-1u);total+=amplitude;
 }
 if(!total)return 0;
 uint32_t value=(weighted/31u)*100u/total;
 return (uint16_t)(value>100u?100u:value);
}

/* Explicit commit. Unchanged or bypass previews create no history entry. */
void etch_editor_sculpt(struct etch_wave_editor *e,uint8_t mode,uint16_t amount,uint8_t pivot){
 if(!e)return;
 int16_t shaped[128];int16_t *dst=e->points[e->wave];
 etch_sculpt_preview(dst,shaped,mode,amount,pivot);
 uint8_t different=0;for(uint32_t i=0;i<128;i++)if(shaped[i]!=dst[i]){different=1;break;}
 if(!different)return;
 etch_editor_checkpoint(e);
 for(uint32_t i=0;i<128;i++)dst[i]=shaped[i];
 e->dirty=1u;++e->revision;
}

/* Keep other two waves, all FX, and the entire preset untouched.
 * A single checkpoint makes this destructive edit undoable.
 * Repeating on an already flat wave is a true no-op. */
void etch_editor_zero_line(struct etch_wave_editor *e){
 if(!e || e->wave>=3u)return;
 uint8_t different=0u;
 for(uint32_t i=0;i<ETCH_EDIT_POINTS;i++)if(e->points[e->wave][i]){different=1u;break;}
 if(!different)return;
 etch_editor_checkpoint(e);
 for(uint32_t i=0;i<ETCH_EDIT_POINTS;i++)e->points[e->wave][i]=0;
 e->dirty=1u;++e->revision;
}
