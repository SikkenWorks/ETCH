/* SPDX-License-Identifier: GPL-3.0-only
 * V1.86 regression: playing an FM1 piano key may not redraw a waveform.
 * Simulate spurious encoder steps from the shared input matrix and verify
 * sustained-note editing remains possible after the brief transition.
 */
#include "../firmware/fm1-native/native_draw_key_guard.h"
#include "../firmware/fm1-native/wave_editor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct etch_wave_editor editor;
static struct etch_draw_key_guard guard;
static int16_t original[3][128];

static void draw_tick(uint32_t ms,uint32_t notes,uint32_t edges,
                      int32_t x,int32_t y,int32_t partial){
 if(etch_draw_key_guard_tick(&guard,notes,edges,ms))x=y=partial=0;
 etch_editor_move_xy128(&editor,x,y,0u);
 if(partial)etch_editor_add_partial(&editor,3u,partial);
}
static void assert_unchanged(void){
 assert(memcmp(original,editor.points,sizeof(original))==0);
 assert(editor.revision==1u);
 assert(editor.undo_count==0u);
}
int main(void){
 etch_editor_init(&editor);
 etch_draw_key_guard_reset(&guard,0u);
 memcpy(original,editor.points,sizeof(original));
 /* Keyboard press and release with false X/Y/harmonic encoder deltas. */
 draw_tick(100u,1u<<4,1u<<4,3,4,2);assert_unchanged();
 draw_tick(125u,1u<<4,0u,1,-4,-2);assert_unchanged();
 draw_tick(150u,1u<<4,0u,-1,2,1);assert_unchanged();
 /* A held key does not permanently lock the waveform editor. */
 draw_tick(175u,1u<<4,0u,0,2,0);
 assert(editor.revision>1u);
 int16_t edited[3][128];memcpy(edited,editor.points,sizeof(edited));
 uint32_t rev=editor.revision;
 /* On key release, discard falsely observed encoder movement again. */
 draw_tick(200u,0u,0u,3,8,2);
 draw_tick(250u,0u,0u,-3,-8,-2);
 assert(editor.revision==rev);
 assert(!memcmp(edited,editor.points,sizeof(edited)));
 /* Normal editing is restored after release, including undo/redo. */
 draw_tick(275u,0u,0u,0,-1,0);
 assert(editor.revision==rev+1u);
 etch_editor_undo(&editor);
 assert(!memcmp(edited,editor.points,sizeof(edited)));
 etch_editor_redo(&editor);
 assert(memcmp(edited,editor.points,sizeof(edited)));
 /* A short note tap between two polls is still noticed via its edge. */
 etch_draw_key_guard_reset(&guard,0u);
 uint32_t rev2=editor.revision;
 draw_tick(500u,0u,1u<<5,0,7,0);
 assert(editor.revision==rev2);
 draw_tick(525u,0u,0u,0,7,0);
 assert(editor.revision==rev2);
 draw_tick(575u,0u,0u,0,1,0);
 assert(editor.revision==rev2+1u);
 /* Re-arming DRAW must clear a previous transition. */
 etch_draw_key_guard_reset(&guard,1u<<5);
 assert(!etch_draw_key_guard_tick(&guard,1u<<5,0u,580u));
 puts("PASS V1.86: piano note-on/off/tap never mutates DRAW; held-note editing and undo/redo preserved");
 return 0;
}
