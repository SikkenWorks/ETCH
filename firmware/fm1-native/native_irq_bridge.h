/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V0.60: USB / AUDIO ISR arbitration from Felucca's proven FM1 model.
 * Pure scheduling state; testable on host. Timer5 has priority 4 and nests
 * ALNK0 (3). Only UAC EP4 may run while the audio ISR owns audio state.
 * General SIE polling (and all MIDI/control traffic) is deferred until the
 * first non-nested Timer5 tick. Never call usb_poll() reentrantly.
 */
#pragma once
#include <stdint.h>
enum { ETCH_IRQ_UAC_SERVICE=1, ETCH_IRQ_USB_POLL=2 };
struct etch_irq_bridge { uint8_t sub, usb_owed; uint32_t last_ticks, milli_remainder; };
static inline void etch_irq_bridge_init(struct etch_irq_bridge *s,uint32_t ticks){
 s->sub=0;s->usb_owed=0;s->last_ticks=ticks;s->milli_remainder=0;
}
static inline uint8_t etch_irq_bridge_tick(struct etch_irq_bridge *s,uint32_t now,uint8_t in_audio,volatile uint32_t *milli){
 /* TIMER4 is 24 MHz; modulo-2^32 subtraction remains valid across wrap. */
 uint32_t delta=now-s->last_ticks;
 s->last_ticks=now;
 uint32_t whole=delta/24000u,rem=delta%24000u;
 s->milli_remainder+=rem;
 if(s->milli_remainder>=24000u){++whole;s->milli_remainder-=24000u;}
 *milli+=whole;
 uint8_t due=(uint8_t)(s->sub%5u==0u);
 if(due)s->usb_owed=1u;
 if(++s->sub==10u)s->sub=0;
 if(in_audio)return due?ETCH_IRQ_UAC_SERVICE:0;
 if(s->usb_owed){s->usb_owed=0;return ETCH_IRQ_USB_POLL;}
 return 0;
}
