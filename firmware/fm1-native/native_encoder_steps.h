/* ETCH V0.92: cutoff alone moves by 20 units per detent; all other
 * parameters retain their previous step sizes. Stored Q15 values and
 * project/patch formats remain unchanged.
 */
#pragma once
#include <stdint.h>
static inline uint16_t etch_encoder_pct(uint16_t raw, int32_t clicks){
 int32_t pct=(int32_t)(((uint32_t)raw*100u)/32767u)+clicks;
 if(pct<0)pct=0; if(pct>100)pct=100;
 /* ceil -> the display's existing floor(raw*100/32767) matches pct */
 return (uint16_t)(((uint32_t)pct*32767u+99u)/100u);
}
static inline uint16_t etch_encoder_unit(uint16_t v,int32_t clicks,uint16_t low,uint16_t high){
 int32_t n=(int32_t)v+clicks;
 if(n<low)n=low;if(n>high)n=high;return (uint16_t)n;
}
/* Cutoff uses 16..8192 device units. Keep the current stored value
 * (including values from older patches); one detent always adds/subtracts 20.
 * Use a wide intermediate so even bad accumulated encoder deltas saturate. */
static inline uint16_t etch_encoder_cutoff(uint16_t value,int32_t clicks){
 int64_t next=(int64_t)value+(int64_t)clicks*20;
 if(next<16)next=16;
 if(next>8192)next=8192;
 return (uint16_t)next;
}
/* On SEQ LOCK, only the cutoff lock (index 2) receives step 20. The
 * remaining five locks retain one raw unit per encoder detent. */
static inline uint16_t etch_encoder_lock_value(uint16_t value,uint8_t lock_index,int32_t clicks){
 int64_t next=(int64_t)value+(int64_t)clicks*(lock_index==2u?20:1);
 if(next<0)next=0;
 if(next>32767)next=32767;
 return (uint16_t)next;
}
static inline uint32_t etch_encoder_hz(uint32_t millihz,int32_t clicks){
 int32_t hz=(int32_t)(millihz/1000u)+clicks;
 if(hz<1)hz=1;if(hz>2000)hz=2000;return (uint32_t)hz*1000u;
}
/* Only K1 and K2 edit Wave Library state. K3/K4 are unused here.
 * Keep this isolated from other pages so their knob actions are unchanged. */
struct etch_library_encoder_state { uint8_t slot, target; };
static inline struct etch_library_encoder_state etch_encoder_library_controls(
 uint8_t slot,uint8_t target,int32_t k1,int32_t k2,int32_t k3,int32_t k4){
 (void)k3;(void)k4;
 int32_t s=(int32_t)slot+k1, t=(int32_t)target+k2;
 if(s<0)s=0;if(s>63)s=63;
 if(t<0)t=0;if(t>2)t=2;
 return (struct etch_library_encoder_state){(uint8_t)s,(uint8_t)t};
}
