#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FELUCCA_OTA 1
static void fm1_delay_ms(uint32_t ms){(void)ms;}
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "../firmware/fm1-native/usb.c"
static uint32_t ota_now_ms(void){return 0;}
static void ota_idle(void){}
static uint32_t packet(uint8_t cin,uint8_t st,uint8_t a,uint8_t b){return cin|((uint32_t)st<<8)|((uint32_t)a<<16)|((uint32_t)b<<24);}
int main(void){
 const uint8_t *d;uint16_t n;assert(get_desc(0x100,&d,&n)&&n==18&&d[7]==64);
 assert(get_desc(0x200,&d,&n)&&n==(d[2]|(d[3]<<8))&&d[4]==2);
 unsigned eps=0;for(unsigned i=0;i<n;){assert(d[i]>=2&&i+d[i]<=n);if(d[i+1]==5){eps++;assert((d[i+2]==1||d[i+2]==0x81)&&d[i+3]==2);}i+=d[i];}assert(eps==2);
 assert(get_desc(0x302,&d,&n)&&n==10&&d[0]==n&&d[2]=='E');
 midi_in_event(packet(9,0x90,60,127));assert(mi_w==1&&midi_in_q[0]==packet(9,0x90,60,127));
 midi_in_event(packet(8,0x90,60,127));midi_in_event(packet(9,0x90,128,127));assert(mi_w==1);
 mi_r=mi_w;for(unsigned i=0;i<MQ;i++)assert(midi_enqueue(packet(9,0x90,60,127),1));assert(!midi_enqueue(0,1)&&midi_in_overflow);
 /* Softkey travels as real USB-MIDI SysEx fragments, not a direct flag. */
 midi_in_event(packet(4,0xf0,0x22,0x24));midi_in_event(packet(7,0x35,0x7d,0xf7));assert(usb.uboot_req);
 usb.uboot_req=0;midi_in_event(packet(4,0xf0,0x22,0x24));midi_in_event(packet(7,0x35,0x7f,0xf7));assert(!usb.uboot_req&&usb.ota_req);
 const uint8_t *frame;uint32_t len;
 sx_ready=0;sysex_byte(0xf0);for(unsigned i=0;i<641;i++)sysex_byte(1);sysex_byte(0xf7);assert(!ota_frame_get(&frame,&len));
 sysex_byte(0xf0);sysex_byte(3);sysex_byte(4);sysex_byte(0xf7);assert(ota_frame_get(&frame,&len)&&len==2&&frame[0]==3&&frame[1]==4);ota_frame_done();assert(!ota_frame_get(&frame,&len));
 puts("USB: descriptor lengths/endpoints/ETCH name, malformed MIDI rejection, overflow, fragmented ROM softkey OTA command and oversized frame rejection passed");
}
