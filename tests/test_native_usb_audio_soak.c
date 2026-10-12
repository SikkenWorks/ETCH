/* 60-second deterministic host simulation of the actual native USB-audio
 * packetizer and ring; the USB controller itself is NOT emulated. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FELUCCA_OTA 1
#define FELUCCA_UAC 1
#define FELUCCA_UAC_48K 0
#define HALF_FRAMES 128u
static void fm1_delay_ms(uint32_t ms){(void)ms;}
static uint32_t ota_now_ms(void){return 0;}
static void ota_idle(void){}
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "../firmware/fm1-native/usb.c"
static uint32_t packets[UA_MAXF];
static int32_t stereo[HALF_FRAMES*2u];
int main(void){
 const uint8_t *desc;uint16_t len;
 assert(get_desc(0x200,&desc,&len));
 assert(desc[4]==3u); /* USB audio-control, MIDI streaming, audio streaming */
 usb.config=1;usb.up=1;uac.alt=1;uac.flowing=1;
 for(uint32_t i=0;i<HALF_FRAMES;i++){
  stereo[2*i]=1100+(int32_t)(i%23u);
  stereo[2*i+1]=-2200-(int32_t)(i%17u);
 }
 /* Actual FM1 I2S observed as about 44,117.6 frames/s; this 44.1176
  * frames/ms producer intentionally drifts against USB's 44.1 frames/ms. */
 uint64_t frame_accum=0,good=0;
 for(uint32_t ms=0;ms<60000u;ms++){
  frame_accum += 441176u;
  while(frame_accum>=HALF_FRAMES*10000u){
   frame_accum-=HALF_FRAMES*10000u;
   uac_render_start();
   if(uac.feed)uac_tap(stereo,HALF_FRAMES);
  }
  uint32_t n=uac_packet(packets);
  assert(n>=43u&&n<=46u);
  if(ms>=250u){
   /* After the initial priming, the ring should handle I2S/USB drift. */
   assert(uac.overruns==0u);
   assert(uac.underruns==0u);
   good++;
  }
 }
 printf("diagnostic: good=%llu pkts=%u underrun=%u overrun=%u fill=%u\n",(unsigned long long)good,uac.pkts,uac.underruns,uac.overruns,uac.fill_min); fflush(stdout);
 assert(good==59750u&&uac.pkts>=59990u);
 assert(uac.adj_up+uac.adj_down>0u);
 assert(uac.fill_hi<UA_N);
 /* Stream stop and restart discard the old ring without old notes leaking. */
 uac_stream(0);uac_render_start();assert(!uac.feed&&!uac.go);
 uac_stream(1);uac.flowing=1;
 uint32_t restart_read=ua_r;
 uac_render_start();assert(uac.go&&uac.feed);
 uac_tap(stereo,HALF_FRAMES);
 assert((ua_ring[(restart_read+UA_PRIME)&(UA_N-1u)]&65535u)==(uint16_t)stereo[0]);
 printf("PASS UAC 60s: pkts=%u frames=%u adj_up=%u adj_down=%u underruns=%u overruns=%u, reconnect\n",
  uac.pkts,uac.frames,uac.adj_up,uac.adj_down,uac.underruns,uac.overruns);
}
