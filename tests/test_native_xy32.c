/* V0.68: physical FM1 DRAW controls use K1 X and K2 Y. */
#include "../firmware/fm1-native/wave_editor.h"
#include <assert.h>
#include <stdio.h>
static struct etch_wave_editor editor;
int main(void){
 etch_editor_init(&editor);
 assert(editor.cursor==0);
 etch_editor_move_xy32(&editor,1,0);assert(editor.cursor==4);
 etch_editor_move_xy32(&editor,100,0);assert(editor.cursor==124);
 etch_editor_move_xy32(&editor,-200,0);assert(editor.cursor==0);
 int16_t orig=editor.points[0][0],untouched=editor.points[0][60];
 etch_editor_move_xy32(&editor,0,1);assert(editor.cursor==0);
 assert(editor.points[0][0]>orig);assert(editor.points[0][60]==untouched);
 assert(editor.undo_count==1);
 etch_editor_undo(&editor);assert(editor.points[0][0]==orig);
 etch_editor_redo(&editor);assert(editor.points[0][0]>orig);
 etch_editor_move_xy32(&editor,0,-1);assert(editor.points[0][0]>=orig-1);
 assert(editor.cursor%4==0 && editor.cursor<=124);
 printf("PASS XY32 K1 horizontal K2 vertical, undo/redo, original 128-wave format\n");
}
