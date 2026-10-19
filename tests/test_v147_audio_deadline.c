/* V1.47 audio DMA safety invariants, pure host regression.
 * No on-target ISR timing can be inferred from this unit test. */
#include <stdio.h>
#include <stdint.h>
#include "../firmware/fm1-native/native_audio_timing.h"
#define REQUIRE(c) do {if(!(c)){fprintf(stderr,"failed at %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
 struct etch_audio_timing a=etch_audio_timing_end(1000u,8500u,1250u,10000u,0u);
 REQUIRE(a.wall_ticks==7500u && a.render_ticks==6250u && !a.late);
 a=etch_audio_timing_end(1000u,11200u,300u,10000u,0u);
 REQUIRE(a.late);
 a=etch_audio_timing_end(0xfffffff0u,0x20u,15u,1000u,0u);
 REQUIRE(a.wall_ticks==48u && a.nested_ticks==15u && a.render_ticks==33u && !a.late);
 a=etch_audio_timing_end(10u,20u,100u,1000u,0u);
 REQUIRE(a.nested_ticks==10u && a.render_ticks==0u);
 a=etch_audio_timing_end(100u,101u,0u,1000u,1u);
 REQUIRE(a.late);
 REQUIRE(etch_audio_dma_preflight(0u,0u,1000u,5000u,9000u));
 REQUIRE(!etch_audio_dma_preflight(0u,1u,1000u,5000u,9000u));
 REQUIRE(!etch_audio_dma_preflight(0u,0u,1000u,6000u,9000u));
 REQUIRE(!etch_audio_dma_preflight(2u,2u,1000u,1100u,9000u));
 REQUIRE(!etch_audio_dma_preflight(1u,1u,1000u,1100u,4096u));
 printf("PASS V1.47 timer wrap, nested IRQ accounting, DMA half and deadline publish guards\n");
 return 0;
}
