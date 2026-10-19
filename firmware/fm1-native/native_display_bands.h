/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH V1.47: bounded UI-only RGB565 transfer planner.
 * Never called from the audio interrupt. No second frame buffer is allocated.
 * The cache is invalidated on page changes, non-composite paints and bounds
 * changes. The original full-width DMA buffers remain in RAM for the transfer.
 */
#ifndef ETCH_NATIVE_DISPLAY_BANDS_H
#define ETCH_NATIVE_DISPLAY_BANDS_H
#include <stdint.h>
#define ETCH_DISPLAY_BANDS 4u
#define ETCH_DISPLAY_WIDTH 240u
#define ETCH_DISPLAY_HEIGHT_MAX 173u
struct etch_display_band_cache {
 uint32_t digest[ETCH_DISPLAY_BANDS];
 uint16_t top, height;
 uint8_t valid;
};
static inline void etch_display_invalidate(struct etch_display_band_cache *c){c->valid=0u;}
static inline uint16_t etch_display_band_start(uint16_t height,uint32_t band){
 return (uint16_t)(((uint32_t)height*band)/ETCH_DISPLAY_BANDS);
}
/* A pair of mixes for the two byte lanes reduces accidental hash collisions;
 * a mismatch triggers an actual pixel transfer, not only a redraw flag. */
static inline uint32_t etch_display_band_hash(const uint16_t *image,uint32_t width,uint32_t begin,uint32_t end){
 uint32_t h=2166136261u;
 for(uint32_t j=begin*width;j<end*width;j++){
  uint32_t px=image[j];
  h=(h^(px&255u))*16777619u;
  h=(h^(px>>8))*16777619u;
 }
 return h;
}
static inline uint8_t etch_display_needs_transfer(const struct etch_display_band_cache *c,
 uint16_t top,uint16_t height,uint32_t index,uint32_t digest){
 return (uint8_t)(!c->valid||c->top!=top||c->height!=height||c->digest[index]!=digest);
}
/* Called after the selected region has reached the panel. */
static inline void etch_display_mark_transfer(struct etch_display_band_cache *c,uint32_t index,uint32_t digest){
 c->digest[index]=digest;
}
static inline void etch_display_finish(struct etch_display_band_cache *c,uint16_t top,uint16_t height){
 c->top=top;c->height=height;c->valid=1u;
}
#endif
