/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ETCH_NATIVE_DRIVE_SOFTCLIP_H
#define ETCH_NATIVE_DRIVE_SOFTCLIP_H
#include <stdint.h>
/* V1.39 phase 2: monotonic, bounded soft knee without a pre-shaper clamp.
 * Input may exceed Q15; all intermediates stay within signed 64-bit.
 * Return Q15, with unity slope below the 0.5 FS knee. */
static inline int32_t etch_drive_softclip(int32_t sample,uint32_t drive){
 if(!drive)return sample;
 if(drive>32767u)drive=32767u;
 int64_t hot=((int64_t)sample*(32768u+3u*drive))/32768;
 /* Keep magnitude in 64-bit until it is bounded. Casting first could
  * wrap an unusually hot input and turn an overload into a small sample. */
 uint64_t magnitude64=(uint64_t)(hot<0?-hot:hot);
 uint32_t magnitude=(uint32_t)(magnitude64>0x3fffffffu?0x3fffffffu:magnitude64);
 const uint32_t knee=16384u,range=16384u;
 if(magnitude>knee){
  uint32_t excess=magnitude-knee;
  /* 32-bit integer division only: no unsupported 64-bit runtime helper. */
  magnitude=knee+range-(range*range)/(range+excess);
 }
 if(magnitude>32767u)magnitude=32767u;
 return hot<0?-(int32_t)magnitude:(int32_t)magnitude;
}
#endif
