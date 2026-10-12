#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_output_startup.h"
int main(void){
 struct etch_output_gate g;
 etch_output_gate_reset(&g,0);
 assert(etch_output_gate_advance(&g,ETCH_OUTPUT_CLOCK_SETTLE_TICKS+1u,0)==ETCH_OUTPUT_WAIT);
 assert(etch_output_gate_advance(&g,ETCH_OUTPUT_CLOCK_SETTLE_TICKS-1u,64)==ETCH_OUTPUT_WAIT);
 assert(etch_output_gate_advance(&g,ETCH_OUTPUT_CLOCK_SETTLE_TICKS,31)==ETCH_OUTPUT_WAIT);
 assert(etch_output_gate_advance(&g,ETCH_OUTPUT_CLOCK_SETTLE_TICKS,32)==ETCH_OUTPUT_ENABLE_ANALOG);
 assert(g.phase==ETCH_OUTPUT_ANALOG_SILENCE);
 assert(etch_output_gate_advance(&g,ETCH_OUTPUT_CLOCK_SETTLE_TICKS+ETCH_OUTPUT_ANALOG_SETTLE_TICKS-1u,400)==ETCH_OUTPUT_WAIT);
 assert(etch_output_gate_advance(&g,ETCH_OUTPUT_CLOCK_SETTLE_TICKS+ETCH_OUTPUT_ANALOG_SETTLE_TICKS,400)==ETCH_OUTPUT_ANALOG_READY);
 assert(g.phase==ETCH_OUTPUT_PLAY);
 assert(etch_output_gate_advance(&g,25000000u,500)==ETCH_OUTPUT_WAIT);
 etch_output_gate_reset(&g,UINT32_MAX-100u);
 assert(etch_output_gate_advance(&g,(uint32_t)(UINT32_MAX-100u+ETCH_OUTPUT_CLOCK_SETTLE_TICKS),32)==ETCH_OUTPUT_ENABLE_ANALOG);
 assert(etch_output_gate_advance(&g,(uint32_t)(UINT32_MAX-100u+ETCH_OUTPUT_CLOCK_SETTLE_TICKS+ETCH_OUTPUT_ANALOG_SETTLE_TICKS),64)==ETCH_OUTPUT_ANALOG_READY);
 uint32_t previous=0;
 for(uint32_t i=0;i<ETCH_OUTPUT_RAMP_SAMPLES;i++){
  assert(i>=previous && i<8192u);previous=i;
 }
 assert(ETCH_OUTPUT_RAMP_SAMPLES==8192u);
 /* Screen may not appear on the hard PC6 edge, nor at beginning or end
  * of PCM ramp. The analog stage must settle independently. */
 struct etch_output_screen_gate s;
 etch_output_screen_reset(&s,100u);
 assert(!etch_output_screen_advance(&s,100u,1,0));
 assert(!etch_output_screen_advance(&s,1000u,1,0));
 assert(!etch_output_screen_advance(&s,2000u,1,1));
 assert(s.saw_analog_ready&&!s.ready);
 assert(!etch_output_screen_advance(&s,2000u+ETCH_OUTPUT_SCREEN_QUIET_TICKS-1u,1,1));
 assert(etch_output_screen_advance(&s,2000u+ETCH_OUTPUT_SCREEN_QUIET_TICKS,1,1));
 assert(!s.timed_out);
 /* PCM must not start when the analog gate is merely ready; the LCD
  * still needs to finish its first large frame. */
 assert(!etch_output_may_unmute(1,1,1,0,0));
 assert(!etch_output_may_unmute(1,0,1,0,1));
 assert(!etch_output_may_unmute(1,1,0,0,1));
 assert(!etch_output_may_unmute(1,1,1,1,1));
 assert(etch_output_may_unmute(1,1,1,0,1));
 /* Startup reset during analog warmup restarts timing, not a premature
  * transition to PLAY. A completed UI transition is latched. */
 assert(etch_output_screen_advance(&s,300000000u,0,0));
 etch_output_screen_reset(&s,0u);
 assert(!etch_output_screen_advance(&s,100u,1,1));
 assert(!etch_output_screen_advance(&s,200u,0,0));
 assert(!s.saw_analog_ready);
 assert(!etch_output_screen_advance(&s,300u,1,1));
 assert(etch_output_screen_advance(&s,300u+ETCH_OUTPUT_SCREEN_QUIET_TICKS,1,1));
 /* A broken DMA link must not leave an infinite boot splash. */
 etch_output_screen_reset(&s,0xFFFFFF00u);
 assert(!etch_output_screen_advance(&s,(uint32_t)(0xFFFFFF00u+ETCH_OUTPUT_SCREEN_TIMEOUT_TICKS-1u),0,0));
 assert(etch_output_screen_advance(&s,(uint32_t)(0xFFFFFF00u+ETCH_OUTPUT_SCREEN_TIMEOUT_TICKS),0,0));
 assert(s.timed_out);
 puts("PASS analog settle, first LCD paint before PCM fade, restart and timeout no-unmute");
 return 0;
}
