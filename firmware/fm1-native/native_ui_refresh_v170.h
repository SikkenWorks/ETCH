/* SPDX-License-Identifier: GPL-3.0-only
 * V1.70: bounded LCD DMA budget when USB audio and IRQs are active.
 * This never throttles the audio callback, MIDI input or parameter publishing.
 */
#pragma once
#include <stdint.h>
static inline uint16_t etch_ui_refresh_interval(uint8_t pressure,uint8_t usb,
                                                uint8_t late){
 if(late>=3u || pressure>=20u)return 150u;
 if(late || pressure>=9u)return 100u;
 if(usb)return 50u;
 return 33u;
}
static inline uint8_t etch_ui_refresh_budget(uint32_t now,uint32_t last,
   uint8_t urgent,uint8_t pressure,uint8_t usb,uint8_t late){
 if(urgent)return 1u;
 return (uint8_t)((uint32_t)(now-last)>=etch_ui_refresh_interval(pressure,usb,late));
}
