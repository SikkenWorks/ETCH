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
  assert(q->tempo==120);
  if(p<16){assert(q->name[0]!='\0');assert(q->attack_ms>=3u);assert(q->release_ms>=180u);}
  else assert(q->name[0]=='E');
 }
 for(unsigned i=0;i<64;i++)assert(bank.sequence[i]==255);
 assert(bank.active[0][4]==bank.slot[0][4]);
 assert(!strcmp(bank.patches[0].name,"WARM KEYS"));
 assert(!strcmp(bank.patches[1].name,"GLASS PAD"));
 assert(!strcmp(bank.patches[2].name,"DUST BASS"));
 puts("V1.70: 16 usable sound starter templates and 16 legacy patches in RAM only");
}
