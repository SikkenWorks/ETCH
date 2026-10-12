/* SPDX-License-Identifier: GPL-3.0-only
 * Read-only probe. A magic signature is evidence of that record type only,
 * not proof of the complete contents. Never writes to flash.
 */
#pragma once
#include <stdint.h>
struct etch_flash_probe {
 uint8_t felucca_sample,used_sectors,io_error;
 uint8_t legacy_sectors,sample2_sectors,sample3_sectors;
};
static inline struct etch_flash_probe etch_flash_probe_read(
    int (*read_cb)(uint32_t,void*,uint32_t)){
 struct etch_flash_probe out={0};
 if(!read_cb){out.io_error=1u;return out;}
 /* Inventory all bytes in legacy and Felucca sample 2+3; the first four
  * bytes are NOT enough to identify partially occupied/corrupt sectors. */
 for(uint32_t off=0x97000u;off<0xDC000u;off+=4096u){
  /* Sample slot 1 0xA0000..0xB4000: only inspect ownership signatures;
   * never use or overwrite it during ETCH takeover. */
  if(off>=0xA1000u&&off<0xB4000u)continue;
  uint8_t bytes[256];uint8_t used=0;
  for(uint32_t pos=0;pos<4096u;pos+=256u){
   if(read_cb(off+pos,bytes,256u)){out.io_error=1u;return out;}
   if(pos==0u&&bytes[0]=='F'&&bytes[1]=='S'&&bytes[2]=='M'&&bytes[3]=='P')out.felucca_sample=1u;
   for(uint32_t j=0;j<256u;j++)if(bytes[j]!=255u){used=1u;break;}
  }
  if(used){out.used_sectors++;
   if(off<0xA1000u)out.legacy_sectors++;
   else if(off<0xC8000u)out.sample2_sectors++;
   else out.sample3_sectors++;
  }
 }
 return out;
}
