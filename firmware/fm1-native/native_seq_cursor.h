/* SPDX-License-Identifier: GPL-3.0-only
 * UI playhead helper: perf_step is the NEXT step, not the current step.
 * Until the first sequencer audio tick, show step zero instead of LEN-1.
 */
#pragma once
#include <stdint.h>
/* Version 0.86: an actual rendered audio step is authoritative. It is not
 * legitimate to subtract 1 from next_step until an audio tick has happened. */
static inline uint16_t etch_seq_actual_step(uint8_t running,uint8_t audio_running,
    uint8_t played,uint8_t current_step,uint16_t length,uint16_t edit_step){
 if(!length||length>256u)return 0u;
 if(!running)return edit_step<length?edit_step:0u;
 if(!audio_running||!played)return 0u;
 return (uint16_t)(current_step%length);
}
static inline uint16_t etch_seq_visible_step(uint8_t running,uint8_t audio_running,
    uint8_t next_step,uint16_t length,uint16_t edit_step){
 if(length<1u||length>256u)return 0u;
 if(!running)return edit_step<length?edit_step:0u;
 if(!audio_running)return 0u;
 return (uint16_t)((((uint16_t)next_step)%length+length-1u)%length);
}
