#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "wave_editor.h"
static void run(void){
 static struct etch_wave_editor e;
 for(int w=0;w<3;w++)for(int i=0;i<128;i++)e.points[w][i]=(int16_t)(w*1000+i+1);
 e.wave=1; e.cursor=27; e.revision=42;
 etch_editor_zero_line(&e);
 assert(e.undo_count==1 && e.redo_count==0 && e.revision==43);
 for(int i=0;i<128;i++){
  assert(e.points[0][i]==i+1);
  assert(e.points[1][i]==0);
  assert(e.points[2][i]==2001+i);
 }
 etch_editor_undo(&e);
 for(int i=0;i<128;i++)assert(e.points[1][i]==1001+i);
 etch_editor_redo(&e);
 for(int i=0;i<128;i++)assert(e.points[1][i]==0);
 int rev=e.revision;
 etch_editor_zero_line(&e);
 assert(e.revision==(uint32_t)rev);
 assert(e.cursor==27 && e.wave==1);
}
int main(void){run(); puts("PASS: editor zero-line/no-op/undo/redo/3-wave isolation");return 0;}
