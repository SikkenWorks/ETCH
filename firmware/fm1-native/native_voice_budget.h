/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V0.70: bounded DSP overload policy (no device I/O, host-testable).
 * Quality reduces only after missed DMA deadlines, never because keys > 8.
 */
#pragma once
#include <stdint.h>
static inline uint8_t etch_voice_budget_reduce(uint8_t voices){
 return voices>6u?6u:4u;
}
static inline uint8_t etch_voice_budget_restore(uint8_t voices){
 return voices<6u?6u:8u;
}
static inline uint8_t etch_voice_budget_valid(uint8_t voices){
 return (uint8_t)(voices==4u||voices==6u||voices==8u);
}
