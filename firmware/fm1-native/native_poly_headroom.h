/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.16: cheap, bounded polyphonic headroom; no RAM/heap/FX changes.
 */
#pragma once
#include <stdint.h>
/* Q10 effective divisor, 4..8 voices. Fast upward response prevents a
 * new dense chord clipping before the old 128-sample headroom slew catches
 * up. Slow recovery prevents gain pumping while releasing a chord. */
static inline int32_t etch_poly_headroom_step(int32_t previous,uint32_t admitted){
 int32_t wanted=(int32_t)(admitted>4u?admitted>8u?8u:admitted:4u)*1024;
 if(previous<4096||previous>8192)previous=4096;
 int32_t delta=wanted-previous;
 if(delta>0){int32_t move=(delta+31)/32;previous+=move;}
 else if(delta<0){int32_t move=(-delta+1023)/1024;previous-=move;}
 return previous;
}
/* Fast, zero-latency peak safety before 16-bit FX input. Below 22k the
 * result is EXACT (no compressor on typical individual voices). Above
 * threshold a continuously-valued soft knee limits the sum without a
 * hard 16-bit plateau. This is deliberately branch-light and avoids
 * divides/multiply-64 in the FM1 44.1 kHz sample ISR.  Worst-case mix
 * magnitude is at most approx 8*32768/4 = 65536 pre drive. */
static inline int32_t etch_poly_safe_sample(int32_t x){
 int32_t mag=x<0?-x:x;
 if(mag<=22000)return x;
 int32_t safe;
 if(mag<=28000)safe=22000+((mag-22000)*3)/4;
 else if(mag<=40000)safe=26500+(mag-28000)/4;
 else safe=29500+(mag-40000)/64;
 if(safe>31744)safe=31744; /* no hard full-scale output */
 return x<0?-safe:safe;
}
