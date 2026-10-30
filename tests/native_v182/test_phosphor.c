#include <assert.h>
#include <stdio.h>
#include "native_phosphor_v182.h"
int main(void){
 assert(etch_phosphor_color(0x0000u)==ETCH_PHOS_BG);
 assert(etch_phosphor_color(0xffffu)==ETCH_PHOS_IVORY);
 assert(etch_phosphor_color(0x67ffu)==ETCH_PHOS_CORAL);
 assert(etch_phosphor_color(0xfde8u)==ETCH_PHOS_LILAC);
 assert(etch_phosphor_color(0xf95bu)==ETCH_PHOS_BLUE);
 assert(etch_phosphor_color(0x07e0u)==ETCH_PHOS_LIME);
 assert(etch_phosphor_color(0xf800u)==ETCH_PHOS_ERROR);
 assert(etch_phosphor_color(ETCH_PHOS_BG)==ETCH_PHOS_BG);
 assert(etch_phosphor_color(ETCH_PHOS_BYPASS)==ETCH_PHOS_BYPASS);
 puts("PASS: PHOSPHOR 565 semantic palette conversions");return 0;
}
