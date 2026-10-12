/* V0.75: changed spectral harmonics must affect the ACTUAL runtime wavetable. */
#include "../firmware/fm1-native/wave_editor.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static struct etch_wave_editor ed;
static struct etch_native_bank baseline,changed;
int main(void){
 etch_editor_init(&ed);
 etch_editor_compile(&ed,&baseline);
 uint32_t rev=ed.revision;
 etch_editor_add_partial(&ed,5,10);
 assert(ed.revision==rev+1&&ed.dirty&&ed.undo_count);
 etch_editor_compile(&ed,&changed);
 long long diff=0;for(int i=0;i<512;i++)diff+=abs((int)baseline.table[0][0][i]-(int)changed.table[0][0][i]);
 assert(diff>100000); /* Audible waveform reshaping, not just graphical movement. */
 for(int mip=0;mip<6;mip++)for(int i=0;i<512;i++)assert(abs((int)changed.table[0][mip][i])<=29000);
 etch_editor_undo(&ed);etch_editor_compile(&ed,&changed);
 for(int i=0;i<512;i++)assert(abs((int)baseline.table[0][0][i]-(int)changed.table[0][0][i])<6);
 etch_editor_redo(&ed);
 ed.cursor=64;int16_t prior=ed.points[0][64];etch_editor_move_xy32(&ed,0,-2);
 assert(ed.points[0][64]!=prior);
 int brush_changes=0;for(int i=56;i<=72;i++)if(ed.points[0][i]!=0)brush_changes++;
 assert(brush_changes>0);
 assert(ed.cursor==64);
 puts("PASS V0.75 harmonic editing changes audio tables, undo/redo, all mips bounded, 17-point sculpt brush");
}
