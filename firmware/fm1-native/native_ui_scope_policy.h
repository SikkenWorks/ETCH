/* SPDX-License-Identifier: GPL-3.0-only
 * Do not amplify released/noisy tail to full-screen amplitude: a PLAY scope
 * with no signal must go to the center line. Pure helper, UI task only.
 */
#pragma once
#include <stdint.h>
static inline uint8_t etch_scope_show_audio(uint8_t signal){return !!signal;}
static inline uint8_t etch_scope_redraw_on_silence(uint8_t signal,uint8_t previous){return signal!=previous;}
static inline uint16_t etch_scope_release_gain(uint16_t auto_gain,uint32_t peak,uint32_t reference){
 if(peak<256u)return 0u;
 if(!reference)return auto_gain;
 uint32_t factor=peak>=reference?256u:(peak*256u)/reference;
 if(factor>256u)factor=256u;
 return (uint16_t)(((uint32_t)auto_gain*factor)/256u);
}

/* V1.17: playback-only oscilloscope release policy. An FX tail may
 * continue to sound after the last key/MIDI/ARP note has been released;
 * it must not keep a static captured waveform on the display forever.
 * No audio, voices, FX buffers or stored waves are modified by this helper.
 * The sweep ends with an EXACTLY zero gain, regardless of offset or the
 * content left in the double-buffered audio capture. */
#define ETCH_SCOPE_RELEASE_MS 260u
struct etch_scope_release_state {
 uint32_t release_started_ms;
 uint8_t had_held_note;
 uint8_t releasing;
};
static inline uint16_t etch_scope_release_step(
 struct etch_scope_release_state *state,uint8_t held_note,uint32_t now_ms){
 if(held_note){state->had_held_note=1u;state->releasing=0u;return 256u;}
 if(state->had_held_note){
  state->had_held_note=0u;state->releasing=1u;
  state->release_started_ms=now_ms;
 }
 if(!state->releasing)return 0u;
 uint32_t elapsed=now_ms-state->release_started_ms; /* unsigned wrap-safe */
 if(elapsed>=ETCH_SCOPE_RELEASE_MS){state->releasing=0u;return 0u;}
 return (uint16_t)(((ETCH_SCOPE_RELEASE_MS-elapsed)*256u)/ETCH_SCOPE_RELEASE_MS);
}
static inline uint16_t etch_scope_visual_gain(uint16_t zoom,uint16_t alpha,
                                               uint8_t signal){
 if(!alpha||!signal)return 0u;
 return (uint16_t)(((uint32_t)zoom*alpha+128u)/256u);
}
/* Redraw on note transitions, during the bounded visual release, or when
 * actual PCM begins/stops. No more endless 20 FPS redraws for an effect
 * feedback tail when all voices are released and the display is flat. */
static inline uint8_t etch_scope_anim_needed(
 const struct etch_scope_release_state *state,uint8_t held_note,
 uint8_t signal,uint8_t last_signal){
 return (uint8_t)(state->had_held_note!=!!held_note || state->releasing ||
                  (!!held_note && !!signal) || signal!=last_signal);
}
