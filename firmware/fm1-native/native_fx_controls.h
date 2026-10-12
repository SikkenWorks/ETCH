/* SPDX-License-Identifier: GPL-3.0-only
 * Turning the cutoff must enable an audible LP filter when patch selected OFF.
 * Cutoff alone uses a fixed 10-unit step for consistent tactile control.
 */
#pragma once
#include <stdint.h>
#include "native_encoder_steps.h"
static inline uint16_t etch_cutoff_turn(uint16_t value,int32_t delta){
 return etch_encoder_cutoff(value,delta);
}
static inline uint8_t etch_filter_enable_if_cutoff_edited(uint8_t type,int32_t turn){
 return (uint8_t)(turn && type==0u ? 1u : type);
}
