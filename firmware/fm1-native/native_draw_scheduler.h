/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.87 adaptive waveform-publish scheduler.
 * Main-loop only: never schedules compilation in the audio interrupt.
 */
#pragma once
#include <stdint.h>
#define ETCH_DRAW_POLL_MS 20u
/* Minimum interval between requests, NOT a measured hardware latency.
 * The three-bank allocator may defer publication while all banks are live.
 * ALNK0's pressure/late feedback takes precedence over editor freshness.
 */
static inline uint16_t etch_draw_compile_interval(uint8_t pressure,
                                                  uint8_t late,uint8_t usb){
 if(late>=3u || pressure>=20u)return 140u;
 if(late || pressure>=9u)return 90u;
 if(usb)return 55u;
 return 40u;
}
static inline uint8_t etch_draw_compile_due(uint32_t now,uint32_t last,
                                            uint8_t drawing,uint8_t dirty,
                                            uint8_t pressure,uint8_t late,
                                            uint8_t usb){
 if(!drawing)return 1u; /* original immediate publish when leaving DRAW */
 if(!dirty)return 0u; /* no work for cursor moves / brush selection */
 return (uint8_t)((uint32_t)(now-last)>=
                       etch_draw_compile_interval(pressure,late,usb));
}
