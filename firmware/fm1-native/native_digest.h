/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
struct etch_sha256 {uint32_t h[8];uint64_t bytes;uint8_t block[64];uint32_t used;};
void etch_sha256_init(struct etch_sha256 *s);
void etch_sha256_add(struct etch_sha256 *s,const uint8_t *p,uint32_t n);
void etch_sha256_end(struct etch_sha256 *s,uint8_t out[32]);
