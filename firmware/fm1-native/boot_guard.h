/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#define ETCH_BOOT_MAGIC 0x45544231u
struct boot_guard {uint32_t magic,pending,failed;};
/* RAM-retained watchdog/restart history, not a durable power-loss counter. */
static uint8_t etch_boot_begin(struct boot_guard *b){
 if(b->magic!=ETCH_BOOT_MAGIC){b->magic=ETCH_BOOT_MAGIC;b->pending=b->failed=0;}
 if(b->pending&&b->failed<2)++b->failed;
 b->pending=1;return b->failed>=2;
}
/* Called only when a healthy audio run has been observed. Never erase
 * previous boot evidence when audio has tripped or has never started. */
static void etch_boot_stable(struct boot_guard *b,uint8_t safe){
 if(!b || b->magic!=ETCH_BOOT_MAGIC)return;
 b->pending=0;if(!safe)b->failed=0;
}
/* A terminal ALNK deadline failure is an explicit fault, even if it occurred
 * after the 30-second boot-stable mark. Do not wait for another boot-loop. */
static void etch_boot_audio_fault(struct boot_guard *b){
 if(!b || b->magic!=ETCH_BOOT_MAGIC)return;
 b->failed=2;b->pending=1;
}
