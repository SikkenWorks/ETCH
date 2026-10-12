#include "../firmware/fm1-native/native_pattern_bank.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static struct etch_pattern_bank bank,old;
static struct etch_native_pattern p,out;
int main(void){
 etch_pattern_bank_clear(&bank);etch_pattern_clear(&p);
 assert(etch_pattern_bank_capture(&bank,0,&p)==0&&bank.used==0);
 assert(etch_pattern_bank_fetch(&bank,0,&out)==0);
 for(int slot=0;slot<32;slot++){
  etch_pattern_clear(&p);
  for(int i=0;i<6;i++)assert(etch_seq_put(&p,(uint16_t)(slot+i*25),(uint8_t)(35+slot),100,(uint8_t)(i+1)));
  p.steps[slot].lock_mask=3;p.steps[slot].locks[0]=1234+slot;p.steps[slot].locks[1]=4321;
  assert(etch_pattern_bank_capture(&bank,(uint8_t)slot,&p)==0);
 }
 assert(!etch_pattern_bank_validate(&bank));
 for(int slot=0;slot<32;slot++){
  assert(etch_pattern_bank_fetch(&bank,(uint8_t)slot,&out)==1);
  assert(out.steps[slot].count==1&&out.steps[slot].notes[0].note==35+slot);
  assert(out.steps[slot].lock_mask==3&&out.steps[slot].locks[0]==1234+slot);
 }
 /* Explicit HOLD survives per-patch serialization; legacy archives still decode. */
 assert(etch_seq_hold_toggle(&p,2,60)==1);
 assert(etch_pattern_bank_capture(&bank,0,&p)==0);
 assert(etch_pattern_bank_fetch(&bank,0,&out)==1&&out.steps[2].hold_count==1&&out.steps[2].holds[0]==60);
 {const uint8_t legacy[]={1,0,0,1,0,60,100,3};
  struct etch_pattern_bank previous={0};previous.lengths[0]=sizeof(legacy);previous.used=sizeof(legacy);
  memcpy(previous.data,legacy,sizeof(legacy));
  assert(!etch_pattern_bank_validate(&previous));assert(etch_pattern_bank_fetch(&previous,0,&out)==1);
  assert(out.steps[0].count==1&&out.steps[0].notes[0].note==60&&out.steps[0].hold_count==0);
 }
 old=bank;
 etch_pattern_clear(&p);
 for(int i=0;i<256;i++){
  for(int j=0;j<8;j++)assert(etch_seq_put(&p,(uint16_t)i,(uint8_t)(48+j),127,8));
  p.steps[i].lock_mask=63;for(int k=0;k<6;k++)p.steps[i].locks[k]=4000+k;
 }
 assert(etch_pattern_bank_capture(&bank,0,&p)==0);/* Explicit HOLD survives per-patch serialization; legacy archives still decode. */
 assert(etch_seq_hold_toggle(&p,2,60)==1);
 assert(etch_pattern_bank_capture(&bank,0,&p)==0);
 assert(etch_pattern_bank_fetch(&bank,0,&out)==1&&out.steps[2].hold_count==1&&out.steps[2].holds[0]==60);
 {const uint8_t legacy[]={1,0,0,1,0,60,100,3};
  struct etch_pattern_bank previous={0};previous.lengths[0]=sizeof(legacy);previous.used=sizeof(legacy);
  memcpy(previous.data,legacy,sizeof(legacy));
  assert(!etch_pattern_bank_validate(&previous));assert(etch_pattern_bank_fetch(&previous,0,&out)==1);
  assert(out.steps[0].count==1&&out.steps[0].notes[0].note==60&&out.steps[0].hold_count==0);
 }
 old=bank;
 assert(etch_pattern_bank_capture(&bank,1,&p)==-2);assert(!memcmp(&bank,&old,sizeof(bank)));
 etch_pattern_clear(&p);assert(etch_pattern_bank_capture(&bank,2,&p)==0);
 assert(etch_pattern_bank_fetch(&bank,2,&out)==0&&bank.used<old.used);
 assert(!etch_pattern_bank_validate(&bank));
 bank.lengths[2]=65535;assert(etch_pattern_bank_validate(&bank)==-1);
 puts("Pattern archive: 32 independent patches, velocities/lengths/locks, resize, capacity refusal without mutation, corruption check PASS");
}
