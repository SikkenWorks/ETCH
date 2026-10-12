/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V0.49 R01: pure, host-testable conservative boot policy.
 * Hardware ROM recovery is a separate prerequisite.
 */
#pragma once
#include <stdint.h>
enum etch_recovery_reason {
 ETCH_RECOVERY_NORMAL=0,
 ETCH_RECOVERY_MANUAL=1,
 ETCH_RECOVERY_BOOT_LOOP=2,
 ETCH_RECOVERY_STORAGE_FAULT=3,
 ETCH_RECOVERY_OTA_CLEANUP=4
};
/* REC (bit 13) at power on enters *application* SAFE, while OCT- still
 * enters the independent chip loader. This deliberately changes the old
 * REC diagnostics from 'muted but writable' to a read-only rescue mode. */
static inline enum etch_recovery_reason etch_recovery_on_boot(uint8_t repeated_boot,uint32_t buttons){
 if(repeated_boot)return ETCH_RECOVERY_BOOT_LOOP;
 if(buttons & (1u<<13))return ETCH_RECOVERY_MANUAL;
 return ETCH_RECOVERY_NORMAL;
}
/* All values other than EMPTY (1) and OK (0) are fail-closed; in particular
 * FOREIGN can mean torn/corrupted ETCH data or another firmware's data. */
static inline uint8_t etch_recovery_storage_is_fault(int status){return status!=0 && status!=1;}
static inline uint8_t etch_recovery_may_write_store(enum etch_recovery_reason why){
 return why==ETCH_RECOVERY_NORMAL;
}
static inline uint8_t etch_recovery_may_run_audio(enum etch_recovery_reason why){
 return why==ETCH_RECOVERY_NORMAL;
}
static inline uint8_t etch_recovery_may_enter_ota(enum etch_recovery_reason why){
 (void)why;return 1; /* Update entry must remain reachable inside SAFE. */
}

/* Stable boot requires actual completed audio callbacks (not merely a live
 * main loop); OTA, forced mute and audio deadline failures never qualify. */
static inline uint8_t etch_recovery_audio_healthy(uint8_t safe,uint8_t tripped,
 uint8_t updating,uint32_t completed_halves){
 return !safe && !tripped && !updating && completed_halves>0;
}
