/* SPDX-License-Identifier: GPL-3.0-only
 * Included in bringup.c after the HAL. Main-loop controls are published in
 * a short IRQ-off critical section. Audio state belongs to ALNK0 only.
 */
#include "audio_core.h"
#include "native_panel_midi.h"
#include "native_audio_timing.h"
#include "native_output_startup.h"
#include "native_performance_midi.h"
#include "native_preset_ramp.h"
static struct etch_native_audio native_audio __attribute__((section(".pool")));
static volatile struct etch_native_controls native_controls;
static int32_t native_dma[ETCH_NATIVE_FRAMES*4] __attribute__((aligned(4)));
static int16_t native_mono[ETCH_NATIVE_FRAMES] __attribute__((section(".noinit")));
static int16_t native_stereo[ETCH_NATIVE_FRAMES*2] __attribute__((section(".noinit")));
#ifndef ETCH_NATIVE_PANEL_MIDI
#define ETCH_NATIVE_PANEL_MIDI 0
#endif
static struct etch_panel_midi native_panel_out;
static struct etch_perf_midi native_perf_out;
#if ETCH_NATIVE_PANEL_MIDI
static int native_panel_usb_emit(uint32_t packet,void *unused){(void)unused;return midi_out_event(packet);}
#endif
#define ETCH_SCOPE_CAPTURE 1024u
/* The 1024-frame double capture is explicitly cleared by native_start()
 * and published only on completed blocks. On UAC builds it shares the
 * dedicated SRAM window with the USB ring, without touching audio DMA,
 * stack, storage, protected .noinit, or the legacy UI canvas. */
#if ETCH_NATIVE_UAC
static int16_t native_scope[2][ETCH_SCOPE_CAPTURE] __attribute__((section(".uac_ram"),aligned(4)));
#else
static int16_t native_scope[2][ETCH_SCOPE_CAPTURE];
#endif
static volatile uint8_t native_scope_ready;
static uint16_t native_scope_cursor;
static volatile uint8_t native_scope_has_signal;
/* Published from ALNK0 at complete capture boundaries; includes MIDI and
 * sequencer voices, not merely front-panel keys. FX tails do not count. */
static volatile uint8_t native_scope_held_note;
static uint32_t native_scope_window_peak;
static uint8_t native_scope_silent_windows;
static volatile uint32_t native_halves,native_late,native_max_ticks;
static volatile uint8_t native_ui_pressure; /* ISR-owned hysteretic LCD budget hint */
static volatile uint32_t native_max_wall_ticks,native_max_nested_ticks;
/* DMA pre-publish guard hits: block writes into a buffer that may be active. */
static volatile uint32_t native_dma_blocked;
static volatile uint8_t native_missed_streak;
static volatile uint16_t native_healthy_blocks;
static volatile uint32_t native_voice_shed_count;
static volatile uint32_t native_audio_peak,native_audio_clip_frames;
static volatile uint8_t native_tripped,native_panic,native_suspended,native_edit_keys_blocked;
/* Capture one external MIDI note into the Project GROUPS editor. The main
 * thread validates/stores the range. Never write flash or project bytes in ISR. */
static volatile uint8_t native_group_capture_active,native_group_midi_pending,native_group_midi_note;
/* The PC6 analog enable is a binary control and may inherently make a
 * hardware bias pop. V1.03 isolates the first large LCD frame from audible
 * PCM: the analog section stabilizes first, the PLAY UI renders in digital
 * silence, then the ISR executes the sample-by-sample fade. This prevents
 * first-frame DMA contention from occurring with nonzero synth audio.
 * It does not promise an analog pop-free physical power enable. */
static volatile uint8_t native_output_enabled;
/* USB attach/suspend can change analog ground/power bias. No claim of
 * eliminating the hardware edge; defer it until bus state is stable. */
static struct etch_output_usb_gate native_usb_gate;
static volatile uint8_t native_output_analog_ready;
static volatile uint8_t native_boot_audio_released;
static volatile uint16_t native_output_fade;
/* The main loop publishes a one-byte request alongside patch control changes.
 * Only ALNK0 touches ramp state. Every preset jump crossfades its outgoing
 * PCM smoothly over 256 samples, including sudden FX/filter changes. */
static volatile uint8_t native_preset_ramp_requested;
static struct etch_preset_ramp native_preset_ramp;
static volatile uint16_t native_prepared_blocks;
static struct etch_output_gate native_output_gate;
/* Main-loop-owned: never synchronized through the realtime callback. */
static struct etch_output_screen_gate native_screen_gate;
static void native_output_poll(void){
 /* GLO-held-at-boot diagnostic runs the SAME normal FM1 audio init with
  * PC6 permanently low. The zeroed DMA remains silent, unlike REC SAFE
  * which skips audio_init entirely. No output enable occurs in this mode. */
 if(native_analog_gate_test)return;
 if(native_suspended||native_tripped||native_output_enabled)return;
 uint32_t now=fm1_ticks();
 uint8_t usb_state=0u;
#if ETCH_NATIVE_USB
 usb_state=(uint8_t)((usb.config?1u:0u)|(usb.suspended?2u:0u));
#endif
 /* PC USB enumeration can repeatedly reconfigure/suspend the link.
  * V1.26 had a 250 ms delay with a 3 s fall-through: PC6 could be
  * enabled immediately after a USB change. Hold DIGITAL ZERO and wait
  * for an uninterrupted stable window instead. This gate is NOT a VBUS
  * voltage or an analogue offset detector. */
 uint8_t usb_stable=etch_output_usb_ready(&native_usb_gate,now,usb_state);
 if(native_output_gate.phase==ETCH_OUTPUT_CLOCK_SILENCE && !usb_stable)return;
 uint8_t action=etch_output_gate_advance(&native_output_gate,fm1_ticks(),native_halves);
 if(action==ETCH_OUTPUT_ENABLE_ANALOG){
  /* The DMA ISR still outputs zeros; no audio reaches the DAC yet. */
  fm1_audio_output_enable(1u);
 }else if(action==ETCH_OUTPUT_ANALOG_READY){
  /* LCD is still showing the splash. Keep the DMA sample stream zero
   * until the first PLAY frame has been completely transferred. */
  native_output_analog_ready=1u;
 }
}

/* Three bank buffers are owned by audio_core; referenced by the UI compiler. */
static struct etch_wave_editor native_editor __attribute__((section(".pool")));
static uint8_t native_draw;
/* SPECTRAL rebuilds are not realtime. Keep the edited points pending until
 * no voices remain; a bank swap transiently doubles oscillator work. */
static uint8_t native_spectral_bank_pending;
static uint8_t native_blend_audio_busy(void);
/* Three already allocated mip banks are reused when GROUPS is enabled:
 * bank 0 = currently edited main sound; bank 1/2 = group preset waves.
 * No new 18-KiB tables and no compilation in the audio callback.
 * The output is muted while changing the bank contents, preventing half-
 * compiled tables from being audible (including any incoming MIDI notes). */
static uint8_t native_groups_compiled;
static uint8_t native_groups_enabled_now(void);
static void native_compile_group_bank(uint8_t index);
static void native_compile_edit(void){
 uint8_t grouped=native_groups_enabled_now();
 if(!native_editor.dirty && grouped==native_groups_compiled){
  native_spectral_bank_pending=0u;return;
 }
 /* All three banks are needed while grouped. Wait for complete note tails.
  * When leaving group mode, also wait for old Group-owned voices. */
 if(grouped || native_groups_compiled){
  if(native_blend_audio_busy())return;
  fm1_irq_off();
  native_controls.bank=0;native_audio.bank=0;native_audio.old_bank=0;native_audio.bank_fade=0;
  fm1_irq_enable_all();
  if(grouped){
   etch_editor_compile(&native_editor,&native_banks[0]);fm1_wdt_feed();
   native_compile_group_bank(0);fm1_wdt_feed();
   native_compile_group_bank(1);fm1_wdt_feed();
   fm1_irq_off();native_controls.bank=&native_banks[0];
   native_audio.bank=&native_banks[0];native_audio.old_bank=0;native_audio.bank_fade=0;fm1_irq_enable_all();
   native_groups_compiled=1u;native_editor.dirty=0u;native_spectral_bank_pending=0u;
   return;
  }
  /* Back to the original three-bank crossfade allocator. */
  native_groups_compiled=0u;native_editor.dirty=1u;
 }
 if(!native_editor.dirty){native_spectral_bank_pending=0u;return;}
 if(native_spectral_bank_pending && (!native_mono_mode || native_blend_audio_busy()))return;
 const struct etch_native_bank *current,*old,*published;
 fm1_irq_off();current=native_audio.bank;old=native_audio.old_bank;published=native_controls.bank;fm1_irq_enable_all();
 for(uint32_t i=0;i<3;i++)if(&native_banks[i]!=current&&&native_banks[i]!=old&&&native_banks[i]!=published){
  etch_editor_compile(&native_editor,&native_banks[i]);
  fm1_irq_off();native_controls.bank=&native_banks[i];fm1_irq_enable_all();
  native_editor.dirty=0;native_spectral_bank_pending=0u;break;
 }
}
#if ETCH_NATIVE_USB
static void native_midi_drain(uint8_t play){
 uint32_t end=mi_w;RING_PUBLISH();
 if(midi_in_overflow){etch_native_midi_panic(&native_audio);mi_r=end;RING_PUBLISH();midi_in_overflow=0;return;}
 for(uint32_t count=0;mi_r!=end&&count<MQ;count++){
  uint32_t pkt=midi_in_q[mi_r%MQ];RING_PUBLISH();++mi_r;
  if(play){
   uint8_t status=(uint8_t)(pkt>>8),note=(uint8_t)(pkt>>16),velocity=(uint8_t)(pkt>>24);
   if(native_group_capture_active && (status&0xf0u)==0x90u && velocity && note<128u){
    if(!native_group_midi_pending){native_group_midi_note=note;native_group_midi_pending=1u;}
    continue;
   }
   if(native_group_capture_active && ((status&0xf0u)==0x80u || (status&0xf0u)==0x90u))continue;
   etch_native_midi(&native_audio,status,note,velocity);
  }
 }
}
#endif
extern void isr_alnk0(void);
static void native_stop(void){native_suspended=1;native_output_enabled=0u;fm1_audio_output_enable(0u);fm1_audio_stop();native_tripped=1;}
static void native_budget_late(void){
 native_healthy_blocks=0;
 if(native_audio.voice_limit>2u){
  native_audio.voice_limit=etch_voice_budget_reduce(native_audio.voice_limit);
  ++native_voice_shed_count;
  native_missed_streak=0;
 }else if(++native_missed_streak>=8u){
  etch_boot_audio_fault(&etch_boot);native_stop();
 }
}
static void native_budget_ok(void){
 native_missed_streak=0;
 if(++native_healthy_blocks>=768u){
  native_healthy_blocks=0;
  if(native_audio.voice_limit<8u)
   native_audio.voice_limit=etch_voice_budget_restore(native_audio.voice_limit);
 }
}
/* Main-only pause: block a leftover ALNK interrupt before transferring ownership. */
static void native_pause(void){
 fm1_irq_off();native_suspended=1;native_output_enabled=0u;fm1_audio_output_enable(0u);fm1_audio_stop();fm1_audio_ack_half();fm1_irq_enable_all();
#if ETCH_NATIVE_PANEL_MIDI
 etch_perf_midi_flush(&native_perf_out,native_panel_usb_emit,0);
#endif
}
void fm1_alnk0_irq(void){
 /* Mark the entire ALNK0 ownership window, not just etch_native_render.
  * TIMER5 nests: only EP4 UAC streaming may be serviced in that window. */
 /* Start a fresh TIMER5 accounting window before allowing nested IRQs.
  * Matches Felucca: wall time controls DMA deadline, nested time is
  * deducted only from the DSP load meter. */
 native_nested_ticks=0u;
 native_in_audio=1u;
 uint8_t p=fm1_audio_pending();fm1_audio_ack_aux(p);
 if(p&FM1_AUDIO_HALF){
  if(native_suspended){fm1_audio_ack_half();native_in_audio=0u;return;}
  uint32_t half=fm1_audio_free_half(),start=fm1_ticks();
  struct etch_native_controls c;
  c=native_controls;
  uint32_t keys=native_edit_keys_blocked?0:fm1_in.notes;
  if(native_panic){native_panic=0;
#if ETCH_NATIVE_USB
mi_r=mi_w;RING_PUBLISH();
#endif
etch_native_reset(&native_audio);native_audio.keys=keys&0x7ffffffu;}
#if ETCH_NATIVE_USB
  native_midi_drain(native_output_enabled?1u:0u);
#endif
#if ETCH_NATIVE_PANEL_MIDI
  etch_panel_midi_update(&native_panel_out,keys,c.octave,(uint8_t)(usb.config && !native_suspended),native_panel_usb_emit,0);
#endif
#if ETCH_NATIVE_UAC
  uac_render_start();
#endif
  /* Boot gate is silent in DIGITAL AND analog phases. Do not expend full
   * voice+FX DSP cycles before the DAC link is stable. A missed DMA deadline
   * while PC6 is switching can force a restart and a second hard pop. */
  native_audio.mono_mode=(uint8_t)(c.mono_mode!=0 && !etch_native_groups_playing());
  if(!native_output_enabled && !native_output_analog_ready){
   for(uint32_t i=0;i<ETCH_NATIVE_FRAMES*2;i++)native_stereo[i]=0;
  }else{
   /* Prime all oscillators/FX while the DMA output remains silent. */
   etch_native_render_stereo(&native_audio,&c,keys,native_stereo,ETCH_NATIVE_FRAMES);
   if(!native_output_enabled && native_prepared_blocks<65535u)native_prepared_blocks++;
  }
#if ETCH_NATIVE_PANEL_MIDI
  etch_perf_midi_update(&native_perf_out,&native_audio,&c,(uint8_t)(usb.config && !native_suspended),ETCH_NATIVE_FRAMES,native_panel_usb_emit,0);
#endif
  /* Reject an expired DMA ownership window BEFORE touching the target DMA
   * buffer. MIDI/render/FX can take long enough for the active half to move.
   * If that happens we stop playback rather than corrupting live samples. */
  if(!etch_audio_dma_preflight(half,fm1_audio_free_half(),start,
      fm1_ticks(),ETCH_NATIVE_FRAMES*544u)){
   fm1_audio_ack_half();
   ++native_dma_blocked;++native_late;
   /* Drop extra voice load (8 > 6 > 4 > 3 > 2) before the watchdog
    * considers stopping DMA; never write to an expired half. */
   native_budget_late();
   native_in_audio=0u;return;
  }
  int32_t *dst=&native_dma[half*ETCH_NATIVE_FRAMES*2];
  if(native_preset_ramp_requested){
   native_preset_ramp_requested=0u;
   etch_preset_ramp_begin(&native_preset_ramp);
  }
  uint32_t block_peak=0,block_clips=0;
  for(uint32_t i=0;i<ETCH_NATIVE_FRAMES;i++){
   int32_t left=native_stereo[2*i],right=native_stereo[2*i+1];
   /* Hold digital silence during codec warmup, then smoothly fade in.
    * 32767*128 cannot overflow signed 32-bit. No division/heap in IRQ. */
   if(!native_output_enabled){left=0;right=0;}
   else if(native_output_fade<ETCH_OUTPUT_RAMP_SAMPLES){
    /* One-sample resolution fade, unlike the former 32-sample staircase. */
    left=(left*(int32_t)native_output_fade)/(int32_t)ETCH_OUTPUT_RAMP_SAMPLES;
    right=(right*(int32_t)native_output_fade)/(int32_t)ETCH_OUTPUT_RAMP_SAMPLES;
    ++native_output_fade;
   }
   /* Avoid hard PCM jumps when scrolling radically different patches.
    * Never touch the analog enable, FX delay buffers or DMA routing. */
   if(native_output_enabled)etch_preset_ramp_frame(&native_preset_ramp,&left,&right);
   uint32_t pl=(uint32_t)(left<0?-left:left),pr=(uint32_t)(right<0?-right:right);
   if(pl>block_peak)block_peak=pl;if(pr>block_peak)block_peak=pr;
   if(pl>=32767u||pr>=32767u)block_clips++;
   native_mono[i]=(int16_t)((left+right)/2);
   dst[2*i]=left*128;dst[2*i+1]=right*128;
  }
#if ETCH_NATIVE_UAC
  /* All output processing is complete in dst. Its Q15 << 7 samples are
   * decoded by the USB tap without a duplicate stereo PCM allocation. */
  if(uac.feed)uac_tap(dst,ETCH_NATIVE_FRAMES);
#endif
  /* Publish a completed 1024-sample contiguous audio window every 8 DMA
   halves. The renderer sees a stable waveform instead of arbitrary 128-sample
   fragments; the audio ISR does no expensive trigger search or UI drawing. */
  uint8_t scope_to=native_scope_ready^1u;
  for(uint32_t i=0;i<ETCH_NATIVE_FRAMES;i++)
   native_scope[scope_to][native_scope_cursor+i]=native_mono[i];
  native_scope_cursor=(uint16_t)(native_scope_cursor+ETCH_NATIVE_FRAMES);
  if(block_peak>native_scope_window_peak)native_scope_window_peak=block_peak;
  if(native_scope_cursor==ETCH_SCOPE_CAPTURE){
   native_scope_cursor=0;native_scope_ready=scope_to;
   uint8_t any_held=0u;
   for(uint8_t vi=0;vi<ETCH_NATIVE_VOICES;vi++){
    const struct etch_native_voice *voice=&native_audio.voice[vi];
    if(voice->active && voice->held){any_held=1u;break;}
   }
   native_scope_held_note=any_held;
   /* A whole 1024-sample window is authoritative. Decay for two
    * complete windows, not every 128-sample ARP gate/rest. */
   if(native_scope_window_peak>256u){native_scope_has_signal=1u;native_scope_silent_windows=0u;}
   else if(native_scope_silent_windows<3u){
    ++native_scope_silent_windows;
    if(native_scope_silent_windows>=3u)native_scope_has_signal=0u;
   }
   native_scope_window_peak=0u;
  }
  if(block_peak>native_audio_peak)native_audio_peak=block_peak;
  if(block_clips>UINT32_MAX-native_audio_clip_frames)native_audio_clip_frames=UINT32_MAX;
  else native_audio_clip_frames+=block_clips;
  fm1_audio_ack_half();++native_halves;
  /* Deadline uses total wall time (USB/input interrupts still consume it).
   * CPU meter excludes TIMER5 time and must not misreport USB work as DSP.
   * A moved DMA half is always a fault, regardless of measured duration. */
  struct etch_audio_timing measure=etch_audio_timing_end(start,fm1_ticks(),
      native_nested_ticks,ETCH_NATIVE_FRAMES*544u,
      (uint8_t)(fm1_audio_free_half()!=half));
  if(measure.render_ticks>native_max_ticks)native_max_ticks=measure.render_ticks;
  if(measure.wall_ticks>native_max_wall_ticks)native_max_wall_ticks=measure.wall_ticks;
  if(measure.nested_ticks>native_max_nested_ticks)native_max_nested_ticks=measure.nested_ticks;
  /* LCD frames are expensive SPI DMA transfers. Track recent wall-time
   * pressure so the UI backs off before repeated audio underruns. This
   * counter never changes oscillator, MIDI or sequencer timing. */
  if(measure.late || measure.wall_ticks>ETCH_NATIVE_FRAMES*544u*65u/100u){
   if(native_ui_pressure<29u)native_ui_pressure+=3u;
   else native_ui_pressure=32u;
  }else if((native_halves&15u)==0u && native_ui_pressure)native_ui_pressure--;
  if(measure.late){
   ++native_late;
   /* A rare late DMA boundary must not disable the instrument forever.
    * The pre-write ownership guard above already rejects expired halves.
    * Escalate only repeated deadlines, and expose LATE/GUARD counters. */
   native_budget_late();
  }else native_budget_ok();
 }
 native_in_audio=0u;
}
static void native_start(void){
 native_suspended=0;native_missed_streak=0;native_healthy_blocks=0;native_tripped=0;native_ui_pressure=0;
 native_output_enabled=0;native_output_analog_ready=0;native_boot_audio_released=0;native_output_fade=0;native_prepared_blocks=0;native_halves=0;
 native_preset_ramp_requested=0u;etch_preset_ramp_reset(&native_preset_ramp);
#if ETCH_NATIVE_USB
 mi_r=mi_w;RING_PUBLISH();midi_in_overflow=0;
#endif
 etch_native_reset(&native_audio);
 native_scope_ready=0;native_scope_cursor=0;native_scope_has_signal=0;native_scope_held_note=0;native_scope_window_peak=0;native_scope_silent_windows=0;etch_output_usb_reset(&native_usb_gate);
 for(uint32_t i=0;i<ETCH_SCOPE_CAPTURE;i++){native_scope[0][i]=0;native_scope[1][i]=0;}
 etch_panel_midi_reset(&native_panel_out);
 for(uint32_t i=0;i<ETCH_NATIVE_FRAMES*4;i++)native_dma[i]=0;
 fm1_audio_output_enable(0u);
 fm1_audio_init(native_dma,ETCH_NATIVE_FRAMES*2,isr_alnk0,3);
 etch_output_gate_reset(&native_output_gate,fm1_ticks());
 /* Every DMA restart must re-arm the silent-first-frame gate. The current
  * UI stays visible on warm restart, but cannot publish audio until after
  * the analog stage has settled and the PLAY renderer has completed. */
 etch_output_screen_reset(&native_screen_gate,fm1_ticks());
}
