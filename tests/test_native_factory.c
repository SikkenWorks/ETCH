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
  else if(p==16)assert(!strcmp(q->name,"STRING ENS"));
  else assert(q->name[0]=='E');
 }
 for(unsigned i=0;i<64;i++)assert(bank.sequence[i]==255);
 assert(bank.active[0][4]==bank.slot[0][4]);
 assert(!strcmp(bank.patches[0].name,"WARM KEYS"));
 assert(!strcmp(bank.patches[1].name,"GLASS PAD"));
 assert(!strcmp(bank.patches[2].name,"DUST BASS"));
 struct etch_native_patch strings;
 etch_factory_strings_patch(&strings);
 assert(!memcmp(&strings,&bank.patches[16],sizeof strings));
 int16_t points[128];
 etch_factory_strings_wave(0,points);
 for(unsigned j=0;j<128;j++)assert(points[j]==bank.slot[1][j]);
 etch_factory_strings_wave(1,points);
 for(unsigned j=0;j<128;j++)assert(points[j]==bank.slot[5][j]);
 assert(strings.attack_ms>=200 && strings.release_ms>=1000);
 assert(strings.chorus>0 && !strings.depth && !strings.drive);
 puts("V1.72: original 16 starter patches intact + STRING ENS at slot 17; audition deterministic");
}
