#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_seq_parity.h"
static struct etch_native_pattern p;
int main(void){
 assert(etch_seq_parity_columns(16)==8);
 assert(etch_seq_parity_columns(64)==8);
 assert(etch_seq_parity_columns(128)==16);
 assert(etch_seq_parity_columns(256)==16);
 assert(etch_seq_parity_rows(8)==1);
 assert(etch_seq_parity_rows(16)==2);
 assert(etch_seq_parity_rows(64)==8);
 assert(etch_seq_parity_rows(128)==8);
 assert(etch_seq_parity_rows(256)==16);
 p.steps[2].count=1;p.steps[2].notes[0]=(struct etch_seq_note){64,100,1};
 assert(etch_seq_parity_note_exists(&p,2,64));
 assert(!etch_seq_parity_note_exists(&p,2,63));
 assert(etch_seq_parity_extend(&p,2,6,16,64));
 assert(p.steps[2].notes[0].length==5);
 uint8_t holds[ETCH_SEQ_STEPS]={0};
 etch_seq_parity_hold_counts(&p,16,holds);
 assert(holds[2]==0 && holds[3]==1 && holds[6]==1 && holds[7]==0);
 p.steps[4].count=1;p.steps[4].notes[0]=(struct etch_seq_note){67,100,3};
 etch_seq_parity_hold_counts(&p,16,holds);
 assert(holds[4]==1 && holds[5]==2 && holds[6]==2 && holds[7]==0);
 assert(!etch_seq_parity_extend(&p,2,1,16,64));
 assert(!etch_seq_parity_extend(&p,2,16,16,64));
 assert(etch_seq_parity_length(0,255,256)==255);
 assert(etch_seq_parity_length(4,9,16)==6);
 puts("PASS V0.81 web/native SEQ grid and held note lengths");
}
