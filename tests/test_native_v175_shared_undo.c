/* SPDX-License-Identifier: GPL-3.0-only
 * V1.75 deterministic reference comparison against V1.74 undo+redo.
 * No hardware/flash operations. Reproduce the previous full-array model.
 */
#include "../firmware/fm1-native/wave_editor.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct legacy_editor {
 int16_t points[3][128],history[16][3][128],redo[16][3][128];
 uint8_t undo_count,undo_head,redo_count;
};
static struct legacy_editor old;
static struct etch_wave_editor editor;
static uint32_t seed=0xEC171575u;
static uint32_t random32(void){seed=1664525u*seed+1013904223u;return seed;}
static void old_checkpoint(void){
 memcpy(old.history[old.undo_head],old.points,sizeof(old.points));
 old.undo_head=(uint8_t)((old.undo_head+1)&15);
 if(old.undo_count<16)old.undo_count++;
 old.redo_count=0;
}
static void old_undo(void){
 if(!old.undo_count)return;
 assert(old.redo_count<16);
 memcpy(old.redo[old.redo_count],old.points,sizeof(old.points));
 ++old.redo_count;
 old.undo_head=(uint8_t)((old.undo_head+15)&15);
 --old.undo_count;
 memcpy(old.points,old.history[old.undo_head],sizeof(old.points));
}
static void old_redo(void){
 if(!old.redo_count)return;
 memcpy(old.history[old.undo_head],old.points,sizeof(old.points));
 old.undo_head=(uint8_t)((old.undo_head+1)&15);
 ++old.undo_count;--old.redo_count;
 memcpy(old.points,old.redo[old.redo_count],sizeof(old.points));
}
static void check(uint32_t i){
 if(memcmp(old.points,editor.points,sizeof(old.points)) ||
    old.undo_count!=editor.undo_count || old.redo_count!=editor.redo_count ||
    old.undo_head!=editor.undo_head){
  fprintf(stderr,"mismatch at operation %u: old=%u/%u new=%u/%u\n",
     i,old.undo_count,old.redo_count,editor.undo_count,editor.redo_count);
  assert(0);
 }
 assert((unsigned)editor.undo_count+(unsigned)editor.redo_count<=16u);
}
int main(void){
 etch_editor_init(&editor);
 memcpy(old.points,editor.points,sizeof(old.points));
 assert(sizeof(editor.history)==12288u);
 assert(sizeof(old.redo)==12288u);
 /* Full 16-step undo/redo, branch replacement, circular overwrite, random walk. */
 for(uint32_t i=0;i<25000u;i++){
  uint32_t r=random32();
  if(i<32u || (r%100u)<55u){
   old_checkpoint();etch_editor_checkpoint(&editor);
   /* Touch multiple waves so this catches partial/incomplete swaps. */
   uint32_t w=random32()%3u,n=random32()%128u;
   int16_t v=(int16_t)(random32()>>16);
   old.points[w][n]=v;editor.points[w][n]=v;
  } else if((r%100u)<80u){old_undo();etch_editor_undo(&editor);}
  else {old_redo();etch_editor_redo(&editor);}
  check(i);
 }
 /* Drain all undo and redo history to exercise 16+16 in both directions. */
 for(uint32_t i=0;i<16u;i++){old_undo();etch_editor_undo(&editor);check(25000u+i);}
 for(uint32_t i=0;i<16u;i++){old_redo();etch_editor_redo(&editor);check(25016u+i);}
 puts("PASS V1.75: 25032 V1.74-equivalent undo/redo operations, all 3 waves; 12,288 bytes saved");
 return 0;
}
