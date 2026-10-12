#include "../firmware/fm1-native/native_audio_timing.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 const uint32_t deadline=128u*544u;
 struct etch_audio_timing t=etch_audio_timing_end(1000,11000,2200,deadline,0);
 assert(t.wall_ticks==10000&&t.nested_ticks==2200&&t.render_ticks==7800&&!t.late);
 /* TIMER5 can consume most of a block; this must not inflate DSP load. */
 t=etch_audio_timing_end(1000,1000+deadline-1,14000,deadline,0);
 assert(t.render_ticks==deadline-1-14000&&!t.late);
 /* Nested USB still consumes real DMA time. Never disguise a deadline miss. */
 t=etch_audio_timing_end(1000,1000+deadline,40000,deadline,0);
 assert(t.late&&t.wall_ticks==deadline&&t.render_ticks==deadline-40000);
 t=etch_audio_timing_end(1000,11000,100,deadline,1);
 assert(t.late); /* DMA half moved, irrespective of timer. */
 /* Guard against accounting corruption/underflow. */
 t=etch_audio_timing_end(2000,2010,1000,deadline,0);
 assert(t.nested_ticks==10&&t.render_ticks==0&&!t.late);
 /* TIMER4 counter wraps across IRQ: fixed-width subtraction stays valid. */
 t=etch_audio_timing_end(0xfffffff0u,0x20u,12,deadline,0);
 assert(t.wall_ticks==0x30u&&t.render_ticks==0x24u&&!t.late);
 t=etch_audio_timing_end(0,1,0,0,0);assert(t.late);
 /* DMA publish preflight: never write into a half already captured by DAC. */
 assert(etch_audio_dma_preflight(0,0,100,110,deadline));
 assert(etch_audio_dma_preflight(1,1,0xfffffff0u,0x20u,deadline));
 assert(!etch_audio_dma_preflight(0,1,100,110,deadline));
 assert(!etch_audio_dma_preflight(1,0,100,110,deadline));
 /* Near the DMA boundary, do not begin publishing a half that may
  * become active during the 128-frame conversion/copy itself. */
 assert(etch_audio_dma_preflight(0,0,100,
   100+deadline-ETCH_AUDIO_DMA_PUBLISH_RESERVE_TICKS-1u,deadline));
 assert(!etch_audio_dma_preflight(0,0,100,
   100+deadline-ETCH_AUDIO_DMA_PUBLISH_RESERVE_TICKS,deadline));
 assert(!etch_audio_dma_preflight(0,0,100,101,
   ETCH_AUDIO_DMA_PUBLISH_RESERVE_TICKS));
 assert(!etch_audio_dma_preflight(0,0,100,100+deadline,deadline));
 assert(!etch_audio_dma_preflight(0,0,100,100+deadline+1u,deadline));
 assert(!etch_audio_dma_preflight(0,0,100,101,0));
 assert(!etch_audio_dma_preflight(2,2,100,101,deadline));
 puts("PASS Felucca ALNK wall/DSP/nested accounting, DMA guard with reserved copy time, wrap");
}
