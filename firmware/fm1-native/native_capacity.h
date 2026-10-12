/* SPDX-License-Identifier: GPL-3.0-only
 * P04: non-destructive capacity verdict. The protected dual-bank region does
 * not contain enough information to justify any expansion of its flash range.
 */
#pragma once
#include <stdint.h>
struct etch_capacity_report { uint32_t required,available,overflow;uint8_t fits; };
static inline struct etch_capacity_report etch_capacity_report(uint32_t need,uint32_t available){
 struct etch_capacity_report c={need,available,need>available?need-available:0,need<=available};
 return c;
}
