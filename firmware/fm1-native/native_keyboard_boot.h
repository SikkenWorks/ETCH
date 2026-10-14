/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.37: dedicated, nonpersistent USB-MIDI keyboard boot policy.
 * SEQ = physical panel button bit 11; OCT- ROM and REC SAFE precede this.
 */
#pragma once
#include <stdint.h>
#define ETCH_BUTTON_SEQ (1u<<11)
#define ETCH_BUTTON_OCT_DOWN (1u<<0)
#define ETCH_BUTTON_OCT_UP (1u<<1)
static inline uint8_t etch_keyboard_boot_requested(uint8_t safe,uint32_t buttons){
 return (uint8_t)(!safe && (buttons&ETCH_BUTTON_SEQ)!=0u &&
                  (buttons&ETCH_BUTTON_OCT_DOWN)==0u);
}
static inline int8_t etch_keyboard_octave_step(int8_t current,uint32_t pressed){
 if((pressed&(ETCH_BUTTON_OCT_DOWN|ETCH_BUTTON_OCT_UP)) ==
    (ETCH_BUTTON_OCT_DOWN|ETCH_BUTTON_OCT_UP))return current;
 if((pressed&ETCH_BUTTON_OCT_DOWN)&&current>-3)return (int8_t)(current-1);
 if((pressed&ETCH_BUTTON_OCT_UP)&&current<3)return (int8_t)(current+1);
 return current;
}
