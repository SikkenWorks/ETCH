/* SPDX-License-Identifier: GPL-3.0-only
 * V1.78: mandatory opt-in choice and separate sustained physical confirmation.
 */
#include <assert.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_storage_boot_prompt.h"
#define SEL (1u<<3)
#define HOME (1u<<8)
#define SAVE (1u<<9)
static struct etch_storage_boot_prompt p;
#define STEP(choose,select,pressed,buttons,time) etch_storage_prompt_input(&p,choose,select,pressed,buttons,time)
int main(void){
 /* Good ETCH storage, genuinely blank storage, safe-mode, and IO failures
  * must never offer any erasure. */
 const int not_foreign[]={ETCH_STORE_OK,ETCH_STORE_EMPTY,ETCH_STORE_IO,ETCH_STORE_VERIFY,ETCH_STORE_FULL,ETCH_STORE_INCOMPATIBLE};
 for(unsigned i=0;i<sizeof(not_foreign)/sizeof(*not_foreign);i++){
  etch_storage_prompt_open(&p,not_foreign[i],1,1);
  assert(p.phase==ETCH_STORAGE_PROMPT_CLOSED);
  assert(STEP(1,0,SEL,SAVE,10000)==ETCH_STORAGE_PROMPT_NONE);
 }
 etch_storage_prompt_open(&p,ETCH_STORE_FOREIGN,0,1);
 assert(p.phase==ETCH_STORAGE_PROMPT_CLOSED);
 etch_storage_prompt_open(&p,ETCH_STORE_FOREIGN,1,0);
 assert(p.phase==ETCH_STORAGE_PROMPT_CLOSED);
 etch_storage_prompt_open(&p,ETCH_STORE_FOREIGN,1,1);
 assert(p.phase==ETCH_STORAGE_PROMPT_CHOOSE && !p.yes);
 /* NO selected by default; SEL confirms NO without any writes. */
 assert(STEP(0,0,SEL,0,10)==ETCH_STORAGE_PROMPT_KEEP);
 assert(p.phase==ETCH_STORAGE_PROMPT_CLOSED);
 etch_storage_prompt_open(&p,ETCH_STORE_FOREIGN,1,1);
 assert(STEP(1,0,0,0,0)==ETCH_STORAGE_PROMPT_NONE && p.yes);
 assert(STEP(0,0,SEL,0,100)==ETCH_STORAGE_PROMPT_NONE);
 assert(p.phase==ETCH_STORAGE_PROMPT_CONFIRM);
 assert(STEP(0,0,0,SAVE,200)==ETCH_STORAGE_PROMPT_NONE);
 assert(STEP(0,0,0,SAVE,9000)==ETCH_STORAGE_PROMPT_NONE); /* no release yet */
 assert(STEP(0,0,0,0,9001)==ETCH_STORAGE_PROMPT_NONE);
 assert(STEP(0,0,0,SAVE,9100)==ETCH_STORAGE_PROMPT_NONE);
 /* Releasing early aborts the countdown. */
 assert(STEP(0,0,0,0,12000)==ETCH_STORAGE_PROMPT_NONE);
 assert(STEP(0,0,0,SAVE,12100)==ETCH_STORAGE_PROMPT_NONE);
 assert(STEP(0,0,0,SAVE,15100)==ETCH_STORAGE_PROMPT_FORMAT);
 /* Home cancels YES, including when SAVE is held. */
 etch_storage_prompt_open(&p,ETCH_STORE_FOREIGN,1,1);
 STEP(1,0,SEL,0,100);
 assert(STEP(0,0,HOME,SAVE|HOME,5000)==ETCH_STORAGE_PROMPT_KEEP);
 assert(STEP(0,0,0,SAVE,9000)==ETCH_STORAGE_PROMPT_NONE);
 etch_storage_prompt_open(&p,ETCH_STORE_FOREIGN,1,1);
 STEP(1,0,SEL,0,100);
 p.phase=ETCH_STORAGE_PROMPT_FAILED;
 assert(STEP(0,0,0,SAVE,5000)==ETCH_STORAGE_PROMPT_NONE);
 assert(STEP(0,0,HOME,HOME,6000)==ETCH_STORAGE_PROMPT_KEEP);
 puts("PASS V1.78 boot modal: default NO, confirmed YES, hold, abort, no writes on faults");
 return 0;
}
