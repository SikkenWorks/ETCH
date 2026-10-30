/* SPDX-License-Identifier: GPL-3.0-only: ETCH V1.85 targeted editor/compile tests. */
#include "../firmware/fm1-native/wave_editor.h"
#include "../firmware/fm1-native/native_ui_wave.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static struct etch_wave_editor ed;
static struct etch_native_bank whole,partial,base;
static unsigned edges(const int16_t *s){unsigned n=0;for(unsigned i=1;i<128;i++)n+=(unsigned)abs(s[i]-s[i-1]);return n;}
int main(void){
 etch_editor_init(&ed);
 assert(etch_ui_draw_cursor_x(0)==0&&etch_ui_draw_cursor_x(127)==239);
 for(unsigned i=1;i<128;i++)assert(etch_ui_draw_cursor_x((uint8_t)i)>etch_ui_draw_cursor_x((uint8_t)(i-1)));
 ed.wave=0;ed.cursor=0;
 etch_editor_move_xy128(&ed,127,0,0);
 assert(ed.cursor==127);
 etch_editor_move_xy128(&ed,-64,0,0);
 assert(ed.cursor==63);
 int16_t snapshot[128];memcpy(snapshot,ed.points[0],sizeof(snapshot));
 etch_editor_move_xy128(&ed,0,2,0);
 assert(ed.points[0][63]!=snapshot[63]);
 assert(ed.points[0][59]==snapshot[59]); /* DETAIL is local */
 etch_editor_undo(&ed);
 assert(!memcmp(snapshot,ed.points[0],sizeof(snapshot)));
 etch_editor_redo(&ed);
 assert(ed.points[0][63]!=snapshot[63]);
 etch_editor_init(&ed);
 etch_editor_compile(&ed,&base);
 /* A sharp waveform corner must create a different timbre and retain
  * high-harmonic texture: EDGE writes only the chosen exact point. */
 ed.wave=0;ed.cursor=41;
 int16_t before[128];memcpy(before,ed.points[0],sizeof(before));
 etch_editor_move_xy128(&ed,0,12,2);
 for(unsigned i=0;i<128;i++)if(i!=41)assert(before[i]==ed.points[0][i]);
 assert(edges(ed.points[0])>edges(before));
 etch_editor_compile(&ed,&whole);
 assert(memcmp(base.table[0],whole.table[0],sizeof(base.table[0])));
 /* Construct a partial rebuild of exactly the same edited source. */
 memcpy(&partial,&base,sizeof(partial));
 etch_editor_compile_wave_source(ed.points,ed.fx,&partial,0);
 assert(!memcmp(&whole,&partial,sizeof(whole)));
 /* Change another waveform only. */
 ed.wave=2;ed.cursor=12;etch_editor_move_xy128(&ed,0,-6,1);
 memcpy(&partial,&whole,sizeof(partial));
 etch_editor_compile_wave_source(ed.points,ed.fx,&partial,2);
 etch_editor_compile(&ed,&whole);
 assert(!memcmp(&whole,&partial,sizeof(whole)));
 /* Group/full compile API remains bit-for-bit equal on all 3 banks. */
 etch_editor_compile_source(ed.points,ed.fx,&partial);
 assert(!memcmp(&whole,&partial,sizeof(whole)));
 puts("PASS V1.85: 128 pixel positions, three brushes, undo/redo, spectrum change, exact partial/full wavetable parity");
}
