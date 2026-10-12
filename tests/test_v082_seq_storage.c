/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_seq_cursor.h"
#include "../firmware/fm1-native/native_flash_probe.h"
static uint8_t mem[0xA000];
static int fault;
static int flash_read(uint32_t off,void *dst,uint32_t n){
 if(fault || off<0x97000u || n>0xA1000u-off)return -1;
 uint8_t *p=dst;off-=0x97000u;
 for(uint32_t i=0;i<n;i++)p[i]=mem[off+i];
 return 0;
}
int main(void){
 assert(etch_seq_visible_step(0,0,0,16,5)==5);
 assert(etch_seq_visible_step(1,0,0,16,5)==0); /* cold start: not last */
 assert(etch_seq_visible_step(1,1,1,16,5)==0);
 assert(etch_seq_visible_step(1,1,0,16,5)==15); /* real loop wrap */
 assert(etch_seq_visible_step(1,1,0,256,5)==255);
 assert(etch_seq_visible_step(1,1,7,8,0)==6);
 assert(etch_seq_visible_step(1,1,0,0,0)==0);
 for(uint32_t i=0;i<sizeof(mem);i++)mem[i]=255;
 struct etch_flash_probe p=etch_flash_probe_read(flash_read);
 assert(!p.used_sectors&&!p.felucca_sample&&!p.io_error);
 /* Felucca sample magic FSMP is located at 0xA0000. */
 const uint32_t m=0xA0000u-0x97000u;
 mem[m]='F';mem[m+1]='S';mem[m+2]='M';mem[m+3]='P';
 p=etch_flash_probe_read(flash_read);
 assert(p.used_sectors==1u&&p.felucca_sample&&!p.io_error);
 /* Unknown, occupied flash is not proof of a Felucca sample. */
 mem[m]='X';p=etch_flash_probe_read(flash_read);
 assert(p.used_sectors==1u&&!p.felucca_sample);
 fault=1;p=etch_flash_probe_read(flash_read);assert(p.io_error);
 puts("V0.82 sequence start/wrap and flash ownership probe: PASS");
 return 0;
}
