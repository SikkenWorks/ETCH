/* V0.44 sequencer editor/LED state regression. */
#include "../firmware/fm1-native/native_seq_ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct etch_native_pattern pat;
int main(void){
 struct etch_seq_key_view v={0};struct etch_seq_grid_view g={0};
 assert(etch_seq_note_cursor(0,99)==0);
 assert(etch_seq_note_cursor(2,7)==1);
 assert(etch_seq_note_cursor(8,6)==6);
 etch_pattern_clear(&pat);
 assert(etch_seq_put(&pat,19,60,88,4));
 assert(etch_seq_put(&pat,19,64,100,2));
 assert(etch_seq_put(&pat,19,67,127,1));
 assert(etch_seq_hold_toggle(&pat,19,60)==1);
 assert(etch_seq_hold_toggle(&pat,19,62)==1);
 pat.steps[19].lock_mask=1u<<3;
 pat.steps[19].locks[3]=22222;
 assert(etch_seq_key_view(&pat,19,0,20,&v)==0);
 assert(v.triggers==((1u<<7)|(1u<<11)|(1u<<14)));
 assert(v.holds==((1u<<7)|(1u<<9)));
 assert(v.selected==(1u<<14)); /* out-of-range cursor selects last voice */
 assert(etch_seq_key_view(&pat,19,1,0,&v)==0);
 assert(v.triggers==((1u<<2))); /* 67 corresponds to key 2 at octave +1 */
 assert(v.selected==0); /* 60 is offscreen at octave +1 */
 assert(etch_seq_key_view(&pat,19,-1,2,&v)==0);
 assert(v.triggers==((1u<<19)|(1u<<23)|(1u<<26)));
 assert(v.selected==(1u<<26));
 assert(etch_seq_key_view(&pat,256,0,0,&v)==-1);
 assert(etch_seq_key_view(0,19,0,0,&v)==-1);
 assert(etch_seq_grid_key_select(19,256,5)==21);
 assert(etch_seq_grid_key_select(19,22,15)==21);
 assert(etch_seq_grid_key_select(19,22,16)==19);
 assert(etch_seq_grid_key_select(0,0,15)==0);
 assert(etch_seq_grid_view(&pat,256,19,&g)==0);
 assert(g.selected==(1u<<3)&&g.triggers==(1u<<3)&&(g.holds&(1u<<3))&&(g.holds&(1u<<4)));
 assert(etch_seq_grid_view(&pat,0,19,&g)==-1);
 assert(etch_seq_step_clear(&pat,19,1)==1);
 assert(pat.steps[19].count==0&&pat.steps[19].hold_count==2);
 assert(pat.steps[19].lock_mask==(1u<<3));
 assert(etch_seq_step_clear(&pat,19,2)==1);
 assert(pat.steps[19].hold_count==0&&pat.steps[19].lock_mask==(1u<<3));
 assert(etch_seq_step_clear(&pat,19,4)==1);
 assert(!pat.steps[19].lock_mask&&!pat.steps[19].locks[3]);
 assert(!etch_seq_step_clear(&pat,19,8));
 assert(etch_seq_note_key_select(&pat,19,72,&(uint8_t){0})==1);
 assert(!etch_seq_step_clear(&pat,256,7));
 /* Direct keyboard entry chooses existing notes without accidental deletion. */
 etch_pattern_clear(&pat);
 uint8_t sel=99;
 assert(etch_seq_note_key_select(&pat,8,60,&sel)==1&&sel==0);
 assert(etch_seq_note_key_select(&pat,8,64,&sel)==2&&sel==1);
 assert(etch_seq_note_key_select(&pat,8,60,&sel)==1&&sel==0&&pat.steps[8].count==2);
 assert(etch_seq_note_key_select(&pat,8,67,&sel)==3&&sel==2);
 assert(etch_seq_note_edit(&pat,8,2,64,100,2)==2); /* duplicate pitch rejected */
 assert(etch_seq_note_remove(&pat,8,2)==1&&pat.steps[8].count==2);
 for(uint8_t note=70;note<76;note++)assert(etch_seq_note_key_select(&pat,8,note,&sel));
 assert(pat.steps[8].count==8&&!etch_seq_note_key_select(&pat,8,76,&sel));
 /* A full-step clear cannot silently orphan an explicitly linked HOLD. */
 assert(etch_seq_hold_link(&pat,16,9,60,8)==1);
 struct etch_seq_step before=pat.steps[8];
 assert(!etch_seq_step_clear(&pat,8,7));
 assert(!memcmp(&before,&pat.steps[8],sizeof(before)));
 assert(etch_seq_hold_unlink(&pat,9,60,8)==1);
 assert(etch_seq_step_clear(&pat,8,7)==1&&pat.steps[8].count==0);
 pat.steps[8].lock_mask=4;
 assert(etch_seq_grid_view(&pat,16,8,&g)==0);
 assert(g.locks==(1u<<8)&&g.selected==(1u<<8));
 puts("PASS V0.44 cursor clamp, 16-key GRID, lock LEDs, chord key-selection (no accidental erase), duplicate/capacity boundaries, safe clear");
}
