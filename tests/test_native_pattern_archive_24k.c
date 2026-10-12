#include "../firmware/fm1-native/native_pattern_bank.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#if !ETCH_P04_EXPANDED
#error extended-store is needed
#endif
static struct etch_pattern_bank bank;
static struct etch_native_pattern pattern,got;
int main(void){
 etch_pattern_bank_clear(&bank);
 for(int slot=0;slot<18;slot++){
  etch_pattern_clear(&pattern);
  for(int step=0;step<40;step++)for(int voice=0;voice<8;voice++)
   assert(etch_seq_put(&pattern,(uint16_t)step,(uint8_t)(40+voice+slot),90,1));
  assert(etch_pattern_bank_capture(&bank,(uint8_t)slot,&pattern)==0);
 }
 assert(bank.used>16000&&bank.used<ETCH_PATTERN_ARCHIVE_BYTES);
 assert(etch_pattern_bank_validate(&bank)==0);
 for(int slot=0;slot<18;slot++){
  assert(etch_pattern_bank_fetch(&bank,(uint8_t)slot,&got)==1);
  assert(got.steps[39].count==8&&got.steps[39].notes[0].note==40+slot);
 }
 printf("PASS V0.53 18 independent poly patterns, %u bytes archive; original 16k limit surpassed\n",bank.used);
}
