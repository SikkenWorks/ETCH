#include "../firmware/fm1-native/native_recovery_policy.h"
#include "../firmware/fm1-native/boot_guard.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct boot_guard b={0};
 assert(!etch_boot_begin(&b));
 assert(etch_recovery_on_boot(0,0)==ETCH_RECOVERY_NORMAL);
 assert(etch_recovery_on_boot(0,1u<<13)==ETCH_RECOVERY_MANUAL);
 assert(etch_recovery_on_boot(1,0)==ETCH_RECOVERY_BOOT_LOOP);
 assert(!etch_boot_begin(&b)); assert(etch_boot_begin(&b));
 assert(etch_recovery_on_boot(1,0)==ETCH_RECOVERY_BOOT_LOOP);
 etch_boot_stable(&b,1);
 assert(b.pending==0 && b.failed==2); /* intentional Safe Mode is not a new crash */
 assert(!etch_recovery_audio_healthy(1,0,0,128));
 assert(!etch_recovery_audio_healthy(0,1,0,128));
 assert(!etch_recovery_audio_healthy(0,0,1,128));
 assert(!etch_recovery_audio_healthy(0,0,0,0));
 assert(etch_recovery_audio_healthy(0,0,0,1));
 /* A late DMA failure must latch SAFE even after the previous good boot. */
 etch_boot_stable(&b,0);
 assert(b.failed==0 && b.pending==0);
 etch_boot_audio_fault(&b);
 assert(b.failed==2 && b.pending==1);
 assert(etch_boot_begin(&b));
 etch_boot_stable(&b,1);
 assert(etch_boot_begin(&b));

 for(int status=-6;status<=1;status++){
  int bad=etch_recovery_storage_is_fault(status);
  assert(bad==(status!=0&&status!=1));
 }
 for(int reason=ETCH_RECOVERY_NORMAL;reason<=ETCH_RECOVERY_OTA_CLEANUP;reason++){
  assert(etch_recovery_may_enter_ota((enum etch_recovery_reason)reason));
  assert(etch_recovery_may_run_audio((enum etch_recovery_reason)reason)==(reason==0));
  assert(etch_recovery_may_write_store((enum etch_recovery_reason)reason)==(reason==0));
 }
 puts("PASS R01 V0.56: healthy-audio boot gate, late DMA failure latch, read-only SAFE, OTA permitted");
}
