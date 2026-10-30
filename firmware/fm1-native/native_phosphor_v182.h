/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.82 PHOSPHOR UI: RGB565 hardware palette. The legacy mapping
 * adapts all existing renderer sites without touching audio/flash code.
 * Explicitly mapped values are display COLORS, not data words.
 */
#ifndef ETCH_PHOSPHOR_V182_H
#define ETCH_PHOSPHOR_V182_H
#include <stdint.h>
#define ETCH_PHOS_BG 0x1083u /* #15121C */
#define ETCH_PHOS_PANEL 0x2105u /* #25202D */
#define ETCH_PHOS_IVORY 0xF75Bu /* #F2E9D8 */
#define ETCH_PHOS_MUTED 0xA4D5u /* #A79AAE */
#define ETCH_PHOS_LIME 0xC78Du /* #C7F36B */
#define ETCH_PHOS_CORAL 0xFBADu /* #FF7668 */
#define ETCH_PHOS_LILAC 0xB4DFu /* #B39AFF */
#define ETCH_PHOS_BLUE 0x865Du /* #82C8E8 */
#define ETCH_PHOS_AMBER 0xFE0Du /* #FFC16E */
#define ETCH_PHOS_BYPASS 0x738Fu /* #77717E */
#define ETCH_PHOS_ERROR 0xFA8Cu /* #FF5263 */
#define ETCH_PHOS_GRID 0x41A9u /* #403749 */
static inline uint16_t etch_phosphor_color(uint16_t old){
 switch(old){
  case 0x0000u: return ETCH_PHOS_BG;
  case 0x1082u: return ETCH_PHOS_PANEL;
  case 0x0C2Bu: return ETCH_PHOS_PANEL;
  case 0x18A4u: return ETCH_PHOS_PANEL;
  case 0x18A3u: return ETCH_PHOS_PANEL;
  case 0x2104u: return ETCH_PHOS_PANEL;
  case 0x2124u: return ETCH_PHOS_GRID;
  case 0x2945u: return ETCH_PHOS_GRID;
  case 0x39C7u: return ETCH_PHOS_GRID;
  case 0x39E7u: return ETCH_PHOS_GRID;
  case 0xFFFFu: return ETCH_PHOS_IVORY;
  case 0xF7DEu: return ETCH_PHOS_IVORY;
  case 0xD7F0u: return ETCH_PHOS_IVORY;
  case 0xDFE4u: return ETCH_PHOS_IVORY;
  case 0xAD55u: return ETCH_PHOS_MUTED;
  case 0x7BEFu: return ETCH_PHOS_MUTED;
  case 0x8410u: return ETCH_PHOS_MUTED;
  case 0x67FFu: return ETCH_PHOS_CORAL;
  case 0x07FFu: return ETCH_PHOS_LIME;
  case 0x07E0u: return ETCH_PHOS_LIME;
  case 0x52AAu: return ETCH_PHOS_LIME;
  case 0xFDE8u: return ETCH_PHOS_LILAC;
  case 0xF95Bu: return ETCH_PHOS_BLUE;
  case 0xFDE0u: return ETCH_PHOS_AMBER;
  case 0xFFE0u: return ETCH_PHOS_AMBER;
  case 0xFD20u: return ETCH_PHOS_AMBER;
  case 0xFDA0u: return ETCH_PHOS_AMBER;
  case 0xF800u: return ETCH_PHOS_ERROR;
  case 0xF81Fu: return ETCH_PHOS_CORAL;
  case 0xF95Fu: return ETCH_PHOS_CORAL;
  default:return old;
 }
}
#endif
