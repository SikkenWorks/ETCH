/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "native_live_preset.h"
int main(void){
 struct etch_live_preset p={0};
 p.magic=ETCH_LIVE_MAGIC;p.version=1;p.bytes=sizeof p;
 p.patch.valid=1;p.patch.slot_a=0;p.patch.slot_b=1;p.patch.slot_c=2;
 p.patch.morph=16384;p.patch.depth=8192;p.patch.sustain=20000;
 p.patch.cutoff=14000;p.patch.filter_type=1;
 p.dsp.spectral_wave=0;p.dsp.spectral_part=2;
 p.harmony.root=0;p.harmony.mode=0;
 assert(etch_live_validate(&p));
 p.patch.slot_a=64;assert(!etch_live_validate(&p));p.patch.slot_a=0;
 p.patch.morph=65535;assert(!etch_live_validate(&p));p.patch.morph=16384;
 p.dsp.organic.drift=101;assert(!etch_live_validate(&p));p.dsp.organic.drift=0;
 p.dsp.blend_pct=101;assert(!etch_live_validate(&p));p.dsp.blend_pct=0;
 p.magic=0;assert(!etch_live_validate(&p));p.magic=ETCH_LIVE_MAGIC;
 p.bytes--;assert(!etch_live_validate(&p));p.bytes++;
 assert(etch_live_validate(&p));
 printf("PASS v177 live preset layout: %zu bytes, corrupt bounds refused\n",sizeof p);
 return 0;
}
