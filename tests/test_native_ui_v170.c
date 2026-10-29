/* ETCH V1.70: 240x240 SOUND page text/bounds and audio-first LCD policy. */
#include "../firmware/fm1-native/native_ui_refresh_v170.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
struct box {uint16_t x0,y0,x1,y1;};
static uint8_t overlap(struct box a,struct box b){
 return (uint8_t)(a.x0<b.x1 && b.x0<a.x1 && a.y0<b.y1 && b.y0<a.y1);
}
static void test_box(struct box b){assert(b.x0<b.x1&&b.y0<b.y1&&b.x1<=240u&&b.y1<=240u);}
int main(void){
 /* All header texts are separated by fixed pixel gutters. */
 const struct box header[]={ {12,9,76,23},{88,14,194,21},{196,12,228,19} };
 for(unsigned i=0;i<3;i++){test_box(header[i]);for(unsigned j=i+1;j<3;j++)assert(!overlap(header[i],header[j]));}
 /* SOUND identity, help line, four parameter columns and visual meters. */
 const struct box footer[]={ {14,166,62,173},{70,166,232,173},
  {14,181,206,188}, {4,198,57,205},{64,198,117,205},
  {124,198,177,205},{184,198,237,205},
  {4,214,57,221},{64,214,117,221},{124,214,177,221},{184,214,237,221},
  {4,233,58,235},{64,233,118,235},{124,233,178,235},{184,233,238,235} };
 for(unsigned i=0;i<sizeof(footer)/sizeof(footer[0]);i++){
  test_box(footer[i]);
  for(unsigned j=i+1;j<sizeof(footer)/sizeof(footer[0]);j++)assert(!overlap(footer[i],footer[j]));
 }
 /* Timer wraps safely. Audio/USB work always wins when LCD is pressured. */
 assert(etch_ui_refresh_interval(0,0,0)==33);
 assert(etch_ui_refresh_interval(0,1,0)==50);
 assert(etch_ui_refresh_interval(9,1,0)==100);
 assert(etch_ui_refresh_interval(20,1,0)==150);
 assert(etch_ui_refresh_interval(0,0,3)==150);
 assert(!etch_ui_refresh_budget(1030,1000,0,20,1,0));
 assert(etch_ui_refresh_budget(1150,1000,0,20,1,0));
 assert(etch_ui_refresh_budget(1001,1000,1,32,1,8));
 assert(etch_ui_refresh_budget(25,UINT32_MAX-24,0,0,1,0));
 puts("PASS V1.70: 240x240 SOUND layout / 4 non-overlapping columns / USB LCD budget");
}
