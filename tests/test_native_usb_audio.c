/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FELUCCA_OTA 1
#define FELUCCA_UAC 1
#define FELUCCA_UAC_48K 0
#define HALF_FRAMES 128u
static void fm1_delay_ms(uint32_t ms){(void)ms;}
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "../firmware/fm1-native/usb.c"
static uint32_t ota_now_ms(void){return 0;}
static void ota_idle(void){}
int main(void){
 const uint8_t *d;uint16_t n;
 assert(get_desc(0x200,&d,&n));assert(n==(uint16_t)(d[2]|d[3]<<8));
 assert(d[4]==3); /* Audio Control, MIDI, Audio Streaming */
 int iso=0,alt=0;
 for(uint32_t i=0;i<n;i+=d[i]){
  assert(d[i]>=2&&i+d[i]<=n);
  if(d[i+1]==5&&d[i+2]==0x84){iso++;assert(d[i+3]==5);}
  if(d[i+1]==4&&d[i+2]==2&&d[i+3]==1)alt++;
 }
 assert(iso==1&&alt==1);
 usb.config=1;uac.alt=1;uac.flowing=1;
 uac_render_start();assert(uac.feed&&uac.go&&ua_w==UA_PRIME);
 int32_t src[HALF_FRAMES*2];for(unsigned i=0;i<HALF_FRAMES;i++){src[i*2]=1234;src[i*2+1]=-2345;}
 uac_tap(src,HALF_FRAMES);assert(ua_w==UA_PRIME+HALF_FRAMES);
 assert(ua_ring[UA_PRIME]==((uint32_t)(uint16_t)1234|((uint32_t)(uint16_t)-2345<<16)));
 uint32_t out[UA_MAXF]={0};for(int k=0;k<20;k++){uint32_t nf=uac_packet(out);assert(nf>=43&&nf<=46);}
 assert(uac.pkts==20&&uac.overruns==0); /* deliberate underruns after 20 packets */
 uac.alt=0;uac_render_start();assert(!uac.feed&&!uac.go);
 puts("USB Audio UAC1: descriptor and 44.1k PCM capture, ring and disconnect PASS");
}
