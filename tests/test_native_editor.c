#include "../firmware/fm1-native/wave_editor.h"
#include "../firmware/fm1-native/audio_core.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static struct etch_wave_editor e;
static struct etch_native_bank a,b;
int main(void){
 etch_editor_init(&e);assert(e.dirty&&e.revision==1);
 etch_editor_select(&e,-1);assert(e.wave==2);etch_editor_select(&e,1);assert(e.wave==0);
 etch_editor_move(&e,-10,0,0,0);assert(e.cursor==0);etch_editor_move(&e,0,1000,0,0);assert(e.cursor==127);
 int16_t old=e.points[0][127];etch_editor_move(&e,0,0,5,0);assert(e.points[0][127]!=old);etch_editor_undo(&e);assert(e.points[0][127]==old);
 etch_editor_init(&e);e.cursor=64;int16_t start=e.points[0][64];
 for(int i=0;i<16;i++)etch_editor_move(&e,0,0,0,1);
 assert(e.undo_count==16);int16_t last=e.points[0][64];
 for(int i=0;i<16;i++)etch_editor_undo(&e);assert(e.points[0][64]==start&&!e.has_undo);
 etch_editor_undo(&e);assert(e.redo_count==16);
 for(int i=0;i<16;i++)etch_editor_redo(&e);assert(e.points[0][64]==last&&e.undo_count==16);
 etch_editor_undo(&e);etch_editor_move(&e,0,0,1,0);assert(!e.redo_count);
 etch_editor_compile(&e,&a);
 for(int w=0;w<3;w++)for(int l=0;l<6;l++){long mean=0;for(int i=0;i<512;i++){assert(abs(a.table[w][l][i])<=29000);mean+=a.table[w][l][i];}assert(labs(mean)<1024);}
 for(int w=0;w<3;w++)for(int i=0;i<128;i++)e.points[w][i]=17000;
 etch_editor_compile(&e,&b);for(int w=0;w<3;w++)for(int l=0;l<6;l++)for(int i=0;i<512;i++)assert(b.table[w][l][i]==0);
 struct etch_native_audio s;struct etch_native_controls c={0,0,32767,0,&a};int16_t out[128];
 etch_native_reset(&s);etch_native_render(&s,&c,1,out,128);assert(s.bank==&a&&s.bank_fade==128);
 etch_native_render(&s,&c,1,out,128);assert(!s.bank_fade);
 c.bank=&b;etch_native_render(&s,&c,1,out,128);assert(s.old_bank==&a&&s.bank_fade==128);
 etch_native_render(&s,&c,1,out,128);assert(!s.bank_fade&&!s.old_bank);
 etch_native_render(&s,&c,1,out,128);for(int i=0;i<128;i++)assert(out[i]==0);
 puts("Native wave editor: directional bounds, A/B/C, undo, DC rejection, mip bounds and 256-sample bank handoff passed");
}
