#include "../firmware/fm1-native/boot_guard.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct boot_guard b={0};
 assert(!etch_boot_begin(&b));
 assert(!etch_boot_begin(&b));
 assert(etch_boot_begin(&b)); /* three early resets: entry to UBOOT */
 etch_boot_stable(&b,1);assert(!b.pending&&b.failed>=2);
 assert(etch_boot_begin(&b)); /* SAFE can't accidentally rearm */
 etch_boot_stable(&b,0);assert(!b.pending&&!b.failed);
 assert(!etch_boot_begin(&b)); /* 30s healthy LCD/scanner run */
 etch_boot_audio_fault(&b);assert(etch_boot_begin(&b));
 puts("PASS Stage0 RAM bootloop watchdog and late-fault loader handoff");
}
