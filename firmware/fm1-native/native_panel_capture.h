/* SPDX-License-Identifier: GPL-3.0-only
 * Exclusive diagnostic input capture: pure control policy, host-testable.
 * This never writes flash and prevents normal navigation during a panel test.
 */
#pragma once
#include <stdint.h>
enum etch_panel_action { ETCH_PANEL_NORMAL=0, ETCH_PANEL_BEGIN=1, ETCH_PANEL_CAPTURE=2, ETCH_PANEL_END=3 };
static inline enum etch_panel_action etch_panel_capture_action(
 uint8_t active,uint8_t panel_page,uint32_t pressed,uint32_t held){
 const uint32_t exit_chord=(1u<<8)|(1u<<9); /* HOME + SAVE */
 if(active)return (held&exit_chord)==exit_chord?ETCH_PANEL_END:ETCH_PANEL_CAPTURE;
 return panel_page&&(pressed&(1u<<13))?ETCH_PANEL_BEGIN:ETCH_PANEL_NORMAL;
}
