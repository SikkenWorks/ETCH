/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca-compatible DMA/CPU measurement. The TIMER5 ISR may run inside
 * ALNK0 to scan inputs and keep USB EP4 alive. Those ticks are non-DSP,
 * but they still count against the hard audio DMA deadline.
 * Pure and host-testable: never touches the audio or USB registers.
 */
#pragma once
#include <stdint.h>
struct etch_audio_timing {
 uint32_t wall_ticks, nested_ticks, render_ticks;
 uint8_t late;
};
static inline struct etch_audio_timing etch_audio_timing_end(
 uint32_t start,uint32_t finish,uint32_t nested_ticks,
 uint32_t deadline_ticks,uint8_t dma_half_moved){
 struct etch_audio_timing x;
 x.wall_ticks=finish-start; /* TIMER4 wraps modulo 2^32. */
 /* Defensive clamp if a corrupted IRQ accounting counter exceeds wall time. */
 x.nested_ticks=nested_ticks>x.wall_ticks?x.wall_ticks:nested_ticks;
 x.render_ticks=x.wall_ticks-x.nested_ticks;
 x.late=(uint8_t)(!deadline_ticks || x.wall_ticks>=deadline_ticks || dma_half_moved);
 return x;
}

/* V0.62: fail closed BEFORE publishing freshly rendered samples into the DMA
 * half. Prior builds checked for a moved half only AFTER writing to it. Once
 * the DMA has moved, that buffer may already be playing. This pure preflight
 * deliberately uses full elapsed (wall) time, never DSP-only timing.
 * We still make the post-copy check: the preflight is not a race-free
 * guarantee, and only an FM1 hardware soak can establish safety margins. */
/* Reserve 4096 TIMER4 ticks (about 171us at 24MHz) for the bounded
 * PCM conversion/publish, USB tap and scope copies AFTER the preflight.
 * Being on the correct DMA half now is not enough if the hardware switches
 * halves halfway through the copy. This is a conservative software guard;
 * physical DMA arbitration still requires a real FM1 soak test. */
#define ETCH_AUDIO_DMA_PUBLISH_RESERVE_TICKS 4096u
static inline uint8_t etch_audio_dma_preflight(
 uint32_t requested_half, uint32_t currently_free_half,
 uint32_t start_ticks, uint32_t now_ticks, uint32_t deadline_ticks){
 return (uint8_t)(deadline_ticks>ETCH_AUDIO_DMA_PUBLISH_RESERVE_TICKS &&
                   requested_half<=1u && currently_free_half==requested_half &&
                   (uint32_t)(now_ticks-start_ticks)<
                   deadline_ticks-ETCH_AUDIO_DMA_PUBLISH_RESERVE_TICKS);
}
