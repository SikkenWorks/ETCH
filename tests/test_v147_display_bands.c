/* V1.47: replay full-width ST7789 band transfers into mock panel memory,
 * compare with a complete redraw after every frame, including forced reset.
 * Runs on host, without invoking HAL, audio or flash.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../firmware/fm1-native/native_display_bands.h"
#define W 240u
#define H 173u
static uint16_t frame[W*H], panel[W*H];
static uint32_t seed=0x41a7c213u;
static uint32_t rnd(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static uint32_t transferred;
static int upload(struct etch_display_band_cache *c, uint16_t top,uint16_t height){
 uint8_t fresh=(uint8_t)(!c->valid||c->top!=top||c->height!=height);
 for(uint32_t b=0;b<ETCH_DISPLAY_BANDS;b++){
  uint32_t lo=etch_display_band_start(height,b),hi=etch_display_band_start(height,b+1u);
  if(hi==lo)continue;
  uint32_t hash=etch_display_band_hash(frame,W,lo,hi);
  if(fresh||etch_display_needs_transfer(c,top,height,b,hash)){
   memcpy(panel+W*lo,frame+W*lo,(hi-lo)*W*sizeof(uint16_t));
   transferred+=(hi-lo)*W*sizeof(uint16_t);
  }
  etch_display_mark_transfer(c,b,hash);
 }
 etch_display_finish(c,top,height);
 return memcmp(frame,panel,height*W*sizeof(uint16_t))==0;
}
int main(void){
 struct etch_display_band_cache cache={0};
 uint32_t first=0,repeated=0,single=0,full=0;
 for(uint32_t i=0;i<W*H;i++)frame[i]=(uint16_t)rnd();
 if(!upload(&cache,67,H))return 1;
 first=transferred;
 transferred=0;
 if(!upload(&cache,67,H))return 2;
 repeated=transferred;
 transferred=0;frame[(H/2)*W+10]^=0x2222u;
 if(!upload(&cache,67,H))return 3;
 single=transferred;
 transferred=0;etch_display_invalidate(&cache);
 if(!upload(&cache,67,H))return 4;
 full=transferred;
 if(first!=W*H*2u||repeated!=0u||single==0u||single>=first||full!=first)return 5;
 for(int step=0;step<2000;step++){
  uint32_t y=rnd()%H;
  uint32_t x=rnd()%W;
  uint32_t n=1u+(rnd()%30u);
  for(uint32_t k=0;k<n&&x+k<W;k++)frame[y*W+x+k]^=(uint16_t)(rnd()|1u);
  if(step%37==0)etch_display_invalidate(&cache);
  if(!upload(&cache,67,H)){fprintf(stderr,"pixel divergence at %d\n",step);return 6;}
 }
 printf("PASS V1.47 display: 2000 exact pixel-matched edits; full=%u bytes, identical=%u, one-pixel=%u, invalidated=%u; %u bounded bands\n",first,repeated,single,full,ETCH_DISPLAY_BANDS);
 return 0;
}
