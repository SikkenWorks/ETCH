/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/native_ui_wave.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 uint32_t valid[2]={1u<<2,(1u<<4)};
 assert(etch_ui_wave_valid(valid,2));assert(etch_ui_wave_valid(valid,36));
 assert(!etch_ui_wave_valid(valid,35));assert(!etch_ui_wave_valid(valid,64));
 assert(etch_ui_wave_next(valid,2,1)==36);
 assert(etch_ui_wave_next(valid,36,1)==2);
 assert(etch_ui_wave_next(valid,2,-1)==36);
 uint32_t empty[2]={0,0};assert(etch_ui_wave_next(empty,7,1)==7);
 int16_t a[128],b[128],out[128];
 for(int i=0;i<128;i++){a[i]=(int16_t)(-30000+i);b[i]=(int16_t)(30000-i);}
 etch_ui_wave_morph(a,b,0,out);for(int i=0;i<128;i++)assert(out[i]==a[i]);
 etch_ui_wave_morph(a,b,32767,out);for(int i=0;i<128;i++)assert(out[i]==b[i]);
 etch_ui_wave_morph(a,b,16384,out);for(int i=0;i<128;i++)assert(out[i]>=-1&&out[i]<=1);
 assert(etch_ui_wave_page_cycle(0,1)==23);
 assert(etch_ui_wave_page_cycle(23,1)==3);
 assert(etch_ui_wave_page_cycle(3,1)==24);
 assert(etch_ui_wave_page_cycle(24,1)==0);
 assert(etch_ui_wave_page_cycle(0,-1)==24);
 assert(etch_ui_wave_page_cycle(24,-1)==3);
 assert(etch_ui_wave_page_cycle(3,-1)==23);
 assert(etch_ui_wave_page_cycle(10,1)==10);
 uint8_t armed=0,page=0;
 assert(etch_ui_wave_load_click(&armed,&page,23,1)==1&&armed==1);
 assert(etch_ui_wave_load_click(&armed,&page,24,1)==1&&page==24);
 assert(etch_ui_wave_load_click(&armed,&page,24,0)==0&&armed==0);
 assert(etch_ui_wave_load_click(&armed,&page,24,1)==1);
 assert(etch_ui_wave_load_click(&armed,&page,24,1)==2&&armed==0);
 assert(etch_ui_draw_cursor_x(0)==0);
 assert(etch_ui_draw_cursor_x(4)==8);
 assert(etch_ui_draw_cursor_x(124)==239);
 assert(etch_ui_draw_cursor_x(127)==239);
 assert(etch_ui_draw_cursor_x(125)<=239);
 assert(etch_ui_text_capacity(0,0)==30);
 assert(etch_ui_text_capacity(232,233)==1);
 assert(etch_ui_text_capacity(233,233)==0);
 assert(etch_ui_text_capacity(0,234)==0);
 /* V0.51: display-only zoom. Silence stays silent; quiet audible waves
    become prominent; samples stay untouched; pixel coordinates stay bounded. */
 int16_t quiet[128],silent[128]={0};for(int i=0;i<128;i++)quiet[i]=(i&1)?700:-700;
 uint16_t zoom=etch_ui_wave_display_gain(quiet,128);
 assert(zoom>256&&zoom<=4096);
 assert(etch_ui_wave_display_gain(silent,128)==256);
 assert(etch_ui_wave_display_y(700,zoom,135,53,80,192)<135);
 assert(etch_ui_wave_display_y(-700,zoom,135,53,80,192)>135);
 assert(etch_ui_wave_display_y(32767,4096,135,53,80,192)==80);
 assert(etch_ui_wave_display_y(-32768,4096,135,53,80,192)==192);
 assert(quiet[0]==-700 && quiet[1]==700);
 puts("PASS V0.46 library validity, wrapped search, preview morph endpoints, and LCD text clipping");
}
