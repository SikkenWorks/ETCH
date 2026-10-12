/* SPDX-License-Identifier: GPL-3.0-only
 * Q15 approximation of ETCH HOST wave-FX processing order.
 * Designed to run off the audio callback when edited waves are recompiled.
 */
#include "native_wave_fx.h"
static int32_t bound(int64_t x){return x<-32768?-32768:x>32767?32767:(int32_t)x;}
static int32_t blend(int32_t a,int32_t b,uint16_t amount){return bound(a+(((int64_t)(b-a)*amount)>>15));}
static uint32_t absi(int32_t x){return (uint32_t)(x<0?-x:x);}
static uint32_t isqrt(uint64_t x){uint64_t bit=1ull<<62,r=0;while(bit>x)bit>>=2;while(bit){if(x>=r+bit){x-=r+bit;r=(r>>1)+bit;}else r>>=1;bit>>=2;}return (uint32_t)r;}
void etch_wave_fx_apply(const int16_t src[128],const struct etch_wave_fx_params *p,int16_t out[128]){
 if(!p){for(uint32_t i=0;i<128;i++)out[i]=src[i];return;}
 uint16_t levels[7]={p->drive,p->fold,p->bitcrush,p->chorus,p->delay,p->reverb,p->smooth};
 uint8_t any=0;for(uint8_t k=0;k<7;k++)any|=levels[k]!=0;
 if(!any||!p->mix){for(uint32_t i=0;i<128;i++)out[i]=src[i];return;}
 int32_t a[128],b[128],dry[128];int64_t dc=0;
 for(uint32_t i=0;i<128;i++)dc+=src[i];dc/=128;
 uint32_t peak=1;for(uint32_t i=0;i<128;i++){a[i]=dry[i]=(int32_t)src[i]-(int32_t)dc;if(absi(a[i])>peak)peak=absi(a[i]);}
 for(uint32_t i=0;i<128;i++)a[i]=dry[i]=bound(((int32_t)a[i]*29000)/(int32_t)peak);
 if(p->drive){
  uint32_t gain=32767u+11u*p->drive;
  for(uint32_t i=0;i<128;i++){
   int32_t x=bound((((int64_t)a[i]*gain)>>15));
   int32_t cubic=(int32_t)(((int32_t)(((int64_t)x*x*x)>>30))/3);
   b[i]=blend(a[i],bound(x-cubic),p->drive);
  }for(uint32_t i=0;i<128;i++)a[i]=b[i];
 }
 if(p->fold){
  for(uint32_t i=0;i<128;i++){
   int64_t x=(((int64_t)a[i]*(32767+5u*p->fold))>>15);
   while(x>32767||x< -32768){if(x>32767)x=65534-x;else x=-65536-x;}
   b[i]=blend(a[i],(int32_t)x,p->fold);
  }for(uint32_t i=0;i<128;i++)a[i]=b[i];
 }
 if(p->bitcrush){
  uint32_t bits=16u-13u*p->bitcrush/32767u;uint32_t step=1u<<(16u-bits);
  uint32_t c=p->bitcrush*255u/32767u;uint32_t hold=1u+(7u*c*c)/(255u*255u);
  int32_t last=0;for(uint32_t i=0;i<128;i++){
   if(i%hold==0){int32_t sample=(a[i]+32768)/(int32_t)step;last=bound((int64_t)sample*step-32768);}
   b[i]=blend(a[i],last,p->bitcrush);
  }for(uint32_t i=0;i<128;i++)a[i]=b[i];
 }
 if(p->chorus){
  uint32_t off=1u+7u*p->chorus/32767u;
  for(uint32_t i=0;i<128;i++){
   int32_t wet=(2*a[i]+a[(i+128-off)&127u]+a[(i+off)&127u])/4;
   b[i]=blend(a[i],wet,(uint16_t)(p->chorus*78u/100u));
  }for(uint32_t i=0;i<128;i++)a[i]=b[i];
 }
 if(p->delay){
  uint32_t off=10u+38u*p->delay/32767u;
  for(uint32_t i=0;i<128;i++){
   int32_t wet=(62*a[(i+128-off)&127u]+25*a[(i+256-2*off)&127u]+13*a[(i+512-3*off)&127u])/100;
   b[i]=blend(a[i],wet,(uint16_t)(p->delay*72u/100u));
  }for(uint32_t i=0;i<128;i++)a[i]=b[i];
 }
 if(p->reverb){
  for(uint32_t i=0;i<128;i++){
   int32_t wet=(100*a[(i+128-3)&127u]+78*a[(i+128-11)&127u]+59*a[(i+128-23)&127u]+43*a[(i+128-37)&127u]+31*a[(i+128-53)&127u])/311;
   b[i]=blend(a[i],wet,(uint16_t)(p->reverb*68u/100u));
  }for(uint32_t i=0;i<128;i++)a[i]=b[i];
 }
 if(p->smooth){
  uint32_t runs=1u+4u*p->smooth/32767u;
  for(uint32_t r=0;r<runs;r++){
   for(uint32_t i=0;i<128;i++)b[i]=(a[(i+127)&127u]+2*a[i]+a[(i+1)&127u])/4;
   for(uint32_t i=0;i<128;i++)a[i]=b[i];
  }
  for(uint32_t i=0;i<128;i++)a[i]=blend(dry[i],a[i],p->smooth);
 }
 uint64_t energy=0;dc=0;peak=1;
 for(uint32_t i=0;i<128;i++){a[i]=blend(dry[i],a[i],p->mix);dc+=a[i];}
 dc/=128;
 for(uint32_t i=0;i<128;i++){a[i]-=(int32_t)dc;peak=absi(a[i])>peak?absi(a[i]):peak;energy+=(uint64_t)((int64_t)a[i]*a[i]);}
 uint32_t rms=isqrt(energy/128u),scale= rms? (uint32_t)((16384u*32768u)/rms):32768u;
 if((uint64_t)scale*peak>29000ull*32768ull)scale=(uint32_t)((29000u*32768u)/peak);
 for(uint32_t i=0;i<128;i++)out[i]=(int16_t)bound(((int64_t)a[i]*scale)/32768ll);
}
