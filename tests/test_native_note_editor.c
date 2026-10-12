/* V0.41 regression: GRID navigation + atomic NOTES edits without HOLD/lock loss. */
#include "../firmware/fm1-native/native_performance.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct etch_native_pattern p;
int main(void){
 assert(etch_seq_grid_move(0,256,0,0,1,0)==8);
 assert(etch_seq_grid_move(8,256,0,0,-1,0)==0);
 assert(etch_seq_grid_move(5,256,1,0,0,0)==21);
 assert(etch_seq_grid_move(16,256,0,1,0,0)==17);
 assert(etch_seq_grid_move(255,256,0,1,0,0)==255);
 assert(etch_seq_grid_move(0,256,-1,0,0,0)==0);
 assert(etch_seq_grid_move(0,8,0,0,1,0)==7);
 assert(etch_seq_grid_move(44,256,INT32_MAX,INT32_MAX,INT32_MAX,INT32_MAX)==255);
 assert(etch_seq_grid_move(44,256,INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN)==0);
 assert(etch_seq_grid_move(5,0,0,0,0,0)==0);
 etch_pattern_clear(&p);
 assert(etch_seq_put(&p,42,60,90,3));
 assert(etch_seq_put(&p,42,64,101,1));
 assert(etch_seq_hold_toggle(&p,42,60)==1);
 p.steps[42].locks[3]=12345;p.steps[42].lock_mask=1u<<3;
 struct etch_seq_step copy=p.steps[42];
 assert(etch_seq_note_edit(&p,42,0,64,127,2)==2); /* collides */
 assert(!memcmp(&copy,&p.steps[42],sizeof(copy)));
 assert(!etch_seq_note_edit(&p,42,5,61,100,1));
 assert(!etch_seq_note_edit(&p,42,0,61,100,0));
 assert(!etch_seq_note_edit(&p,256,0,61,100,1));
 assert(etch_seq_note_edit(&p,42,0,61,127,2)==1);
 assert(p.steps[42].notes[0].note==61 && p.steps[42].notes[0].velocity==127);
 assert(p.steps[42].notes[0].length==2);
 assert(p.steps[42].hold_count==1&&p.steps[42].holds[0]==60);
 assert(p.steps[42].lock_mask==(1u<<3)&&p.steps[42].locks[3]==12345);
 assert(etch_seq_note_remove(&p,42,0)==1);
 assert(p.steps[42].count==1&&p.steps[42].notes[0].note==64);
 assert(!etch_seq_note_remove(&p,42,1));
 assert(p.steps[42].hold_count==1&&p.steps[42].holds[0]==60);
 puts("PASS V0.41 GRID 8-column navigation; atomic note edit and delete; HOLD/lock preservation");
 return 0;
}
