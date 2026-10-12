#include "../firmware/fm1-native/native_factory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct etch_wave_library bank;
int main(void){
 etch_factory_seed(&bank);
 assert(bank.valid[0]==0xffffu&&bank.valid[1]==0);
 assert(!strcmp(bank.wave_names[0],"SINE")&&!strcmp(bank.wave_names[9],"TEETH"));
 for(unsigned p=0;p<32;p++){
  const struct etch_native_patch *q=&bank.patches[p];
  assert(q->valid&&q->slot_a<16&&q->slot_b<16&&q->slot_c<16);
  assert(q->name[0]=='E'&&q->tempo==120);
 }
 for(unsigned i=0;i<64;i++)assert(bank.sequence[i]==255);
 assert(bank.active[0][4]==bank.slot[0][4]);
 puts("Native factory: 16 editable starter waves and 32 named patches seeded in RAM only");
}
