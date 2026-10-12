#include "../firmware/fm1-native/native_irq_bridge.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct etch_irq_bridge s={0};uint32_t millis=0;
 etch_irq_bridge_init(&s,0xFFFFFF00u);
 uint32_t t=0xFFFFFF00u;
 /* 50 ms of TIMER5 ticks: alternating 3.0 ms nested and 2.0 ms idle.
  * No general usb_poll may ever run nested, while EP4 keeps its 2 kHz slot.
  * This includes a wrap of the 24 MHz free-running timer. */
 uint32_t polls=0,ep4=0,nested=0;
 for(unsigned i=0;i<500;i++){
   t+=2400u;uint8_t audio=(uint8_t)((i%50u)<30u);
   uint8_t act=etch_irq_bridge_tick(&s,t,audio,&millis);
   if(audio){assert(!(act&ETCH_IRQ_USB_POLL));if(act&ETCH_IRQ_UAC_SERVICE)ep4++;nested++;}
   else {assert(!(act&ETCH_IRQ_UAC_SERVICE));if(act&ETCH_IRQ_USB_POLL)polls++;}
 }
 assert(millis==50 && ep4==60 && polls>0 && polls<=40 && nested==300);
 /* Missed general USB poll must be paid back on the first un-nested scan. */
 etch_irq_bridge_init(&s,5000000u);millis=0;t=5000000u;
 for(unsigned i=0;i<35;i++){t+=2400u;assert(etch_irq_bridge_tick(&s,t,1,&millis)!=ETCH_IRQ_USB_POLL);}
 t+=2400u;assert(etch_irq_bridge_tick(&s,t,0,&millis)==ETCH_IRQ_USB_POLL);
 /* Extended 60-second equivalent: jittered TIMER5 ticks and a steady
  * 3ms audio / 2ms idle cadence. No SIE poll while nested, no starvation
  * when the next non-nested tick arrives. Feed 600,000 ticks at 10kHz. */
 etch_irq_bridge_init(&s,0xffffff00u);t=0xffffff00u;millis=0;
 uint32_t long_ep4=0,long_poll=0,last_poll_tick=0;
 for(uint32_t i=0;i<600000u;i++){
  /* Preserve the 100us average with alternating 80/120us periods. */
  t+=(i&1u)?2880u:1920u;
  uint8_t in_audio=(uint8_t)((i%50u)<30u);
  uint8_t a=etch_irq_bridge_tick(&s,t,in_audio,&millis);
  if(in_audio){assert(a!=ETCH_IRQ_USB_POLL);if(a==ETCH_IRQ_UAC_SERVICE)long_ep4++;}
  else {assert(a!=ETCH_IRQ_UAC_SERVICE);if(a==ETCH_IRQ_USB_POLL){long_poll++;assert(i-last_poll_tick<=55u);last_poll_tick=i;}}
 }
 assert(millis==60000u && long_ep4==72000u && long_poll==48000u);
 puts("PASS Felucca nested USB EP4/SIE, 60s jittered scan and TIMER4 wrap");
}
