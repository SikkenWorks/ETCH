/* SPDX-License-Identifier: GPL-3.0-only
 * V1.87 main-thread DRAW compile decisions and overload safety.
 */
#include "../firmware/fm1-native/native_draw_scheduler.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(ETCH_DRAW_POLL_MS==20u);
 assert(etch_draw_compile_interval(0,0,0)==40u);
 assert(etch_draw_compile_interval(0,0,1)==55u);
 assert(etch_draw_compile_interval(9,0,0)==90u);
 assert(etch_draw_compile_interval(20,0,0)==140u);
 assert(etch_draw_compile_interval(0,3,0)==140u);
 assert(!etch_draw_compile_due(139,100,1,1,0,0,0));
 assert(etch_draw_compile_due(140,100,1,1,0,0,0));
 assert(!etch_draw_compile_due(140,100,1,0,0,0,0));
 assert(!etch_draw_compile_due(150,100,1,1,0,0,1));
 assert(etch_draw_compile_due(155,100,1,1,0,0,1));
 assert(!etch_draw_compile_due(180,100,1,1,10,0,0));
 assert(etch_draw_compile_due(190,100,1,1,10,0,0));
 assert(!etch_draw_compile_due(230,100,1,1,20,0,0));
 assert(etch_draw_compile_due(240,100,1,1,20,0,0));
 /* Leaving DRAW / loading patches cannot wait for a throttle window. */
 assert(etch_draw_compile_due(101,100,0,1,30,8,1));
 assert(etch_draw_compile_due(101,100,0,0,30,8,1));
 /* wrapping free-running tick counter must preserve timing. */
 assert(!etch_draw_compile_due(9,0xfffffff0u,1,1,0,0,0));
 assert(etch_draw_compile_due(24,0xfffffff0u,1,1,0,0,0));
 puts("PASS V1.87: adaptive waveform scheduling, dirty gating, pressure/USB, exit DRAW, tick wrap");
 return 0;
}
