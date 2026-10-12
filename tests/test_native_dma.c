#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define FM1_AUDIO_HALF 0x80u
static struct {uint32_t notes;} fm1_in;
static uint32_t half,ticks,step,stopped,acked,started;
static uint8_t analog_enabled;
static void fm1_audio_output_enable(uint8_t enabled){analog_enabled=enabled;}
static uint8_t fm1_audio_pending(void){return FM1_AUDIO_HALF;}
static void fm1_audio_ack_aux(uint8_t p){(void)p;}
static uint32_t fm1_audio_free_half(void){return half;}
static void fm1_audio_ack_half(void){acked++;}
static uint32_t fm1_ticks(void){uint32_t n=ticks;ticks+=step;return n;}
static void fm1_audio_stop(void){stopped++;}
static void fm1_audio_init(int32_t *buf,uint32_t words,void (*isr)(void),uint32_t priority){
 assert(words==256&&priority==3&&isr);for(int i=0;i<512;i++)assert(buf[i]==0);started++;
}
void isr_alnk0(void){}
static void fm1_irq_off(void){}
static void fm1_irq_enable_all(void){}
#define ETCH_NATIVE_USB 1
#define MQ 64u
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static uint32_t midi_in_q[MQ];
static volatile uint32_t mi_w,mi_r;
static volatile uint8_t midi_in_overflow;
static struct {uint8_t config,suspended;} usb;
#include "../firmware/fm1-native/boot_guard.h"
static struct boot_guard etch_boot={.magic=ETCH_BOOT_MAGIC,.pending=1,.failed=0};
static volatile uint8_t native_in_audio; /* shared ALNK0 / TIMER5 handshake */
static volatile uint32_t native_nested_ticks; /* TIMER5 accumulates inside ALNK0 */
#include "../firmware/fm1-native/audio_target.c"
int main(void){
 etch_editor_init(&native_editor);native_compile_edit();assert(native_controls.bank==&native_banks[0]);
 native_controls.gain=32767;native_start();assert(started==1&&!analog_enabled);
 /* PC6 must remain muted during 600ms silent I2S startup. Digital audio
  * must remain silent for another 250ms AFTER the analog gate opens. */
 fm1_in.notes=0;for(uint32_t k=0;k<ETCH_OUTPUT_MIN_DMA_HALVES;k++)fm1_alnk0_irq();
 native_output_poll();assert(!analog_enabled && !native_output_enabled);
 ticks+=ETCH_OUTPUT_CLOCK_SETTLE_TICKS;native_output_poll();
 assert(analog_enabled && !native_output_enabled);
 fm1_in.notes=1;
 for(int j=0;j<2;j++){fm1_alnk0_irq();for(int i=0;i<256;i++)assert(native_dma[i]==0);}
 ticks+=ETCH_OUTPUT_ANALOG_SETTLE_TICKS-1u;native_output_poll();assert(!native_output_enabled);
 ticks+=1u;native_output_poll();
 /* The analog codec is settled, but the first LCD frame must finish before
  * a single nonzero PCM sample is allowed through the DMA buffer. */
 assert(native_output_analog_ready && !native_output_enabled);
 for(int j=0;j<2;j++){
  fm1_alnk0_irq();
  for(int i=0;i<256;i++)assert(native_dma[i]==0);
 }
 assert(!etch_output_may_unmute(1,1,1,0,0));
 assert(etch_output_may_unmute(1,1,1,0,1));
 native_output_fade=0u;
 native_output_enabled=1u;
 native_boot_audio_released=1u;
 uint32_t first_acks=acked;
 for(int j=0;j<5;j++){fm1_alnk0_irq();assert(native_in_audio==0);}assert(acked==first_acks+5);int audible=0;
 for(int i=0;i<128;i++){assert(native_dma[2*i]==native_dma[2*i+1]);assert(native_dma[2*i]%128==0);audible|=native_dma[2*i]!=0;}assert(audible);
 /* Hardware adapter must no longer mirror the mono FX return to both pins.
  * With wet chorus+delay+reverb engaged, DMA channels genuinely diverge. */
 native_controls.chorus=32767;native_controls.delay=24000;native_controls.reverb=27000;
 unsigned stereo_changed=0;
 for(int block=0;block<32;block++){
  fm1_alnk0_irq();
  for(int i=0;i<128;i++)if(native_dma[2*i]!=native_dma[2*i+1])stereo_changed++;
 }
 assert(stereo_changed>100);
 native_controls.chorus=native_controls.delay=native_controls.reverb=0;
 native_panic=1;fm1_alnk0_irq();for(int i=0;i<256;i++)assert(native_dma[i]==0);
 fm1_alnk0_irq();for(int i=0;i<256;i++)assert(native_dma[i]==0);
 fm1_in.notes=0;fm1_alnk0_irq();fm1_in.notes=1;fm1_alnk0_irq();
 half=1;fm1_alnk0_irq();assert(native_dma[256]!=0||native_dma[258]!=0);
 step=128*544;
 for(int missed=0;missed<10;missed++){
  fm1_alnk0_irq();
  if(missed<9)assert(stopped==0 && !native_tripped);
  if(missed==0)assert(native_audio.voice_limit==6);
  if(missed==1)assert(native_audio.voice_limit==4);
 }
 assert(!analog_enabled);assert(stopped==1 && native_tripped && native_late==10 && native_dma_blocked==10);
 assert(etch_boot.failed==2 && etch_boot.pending==1);
 step=0;half=0;fm1_in.notes=0;native_start();assert(!analog_enabled);
 midi_in_q[mi_w++%MQ]=9u|(0x90u<<8)|(60u<<16)|(100u<<24);
 fm1_alnk0_irq();assert(mi_r==mi_w && !native_audio.voice[0].held); /* discard stale startup MIDI */
 native_output_enabled=1u; /* enable after the tested UI/analog gate */
 midi_in_q[mi_w++%MQ]=9u|(0x90u<<8)|(60u<<16)|(100u<<24);
 fm1_alnk0_irq();assert(native_audio.voice[0].note==188&&native_audio.voice[0].held);
 midi_in_overflow=1;fm1_alnk0_irq();assert(!native_audio.voice[0].held&&!midi_in_overflow&&mi_r==mi_w);
 midi_in_q[mi_w++%MQ]=9u|(0x90u<<8)|(61u<<16)|(100u<<24);native_midi_drain(0);assert(mi_r==mi_w);
 midi_in_q[mi_w++%MQ]=9u|(0x90u<<8)|(62u<<16)|(100u<<24);native_start();assert(mi_r==mi_w);
 uint32_t rendered=native_halves;native_pause();assert(native_suspended);fm1_alnk0_irq();assert(!native_in_audio&&native_halves==rendered);native_start();assert(!native_suspended);
 /* USB host enumeration changing during the clock-silent phase must NOT
  * switch PC6 immediately, but must also never strand the startup gate. */
 for(unsigned j=0;j<ETCH_OUTPUT_MIN_DMA_HALVES;j++)fm1_alnk0_irq();
 usb.config=0;native_output_poll();
 ticks+=ETCH_OUTPUT_CLOCK_SETTLE_TICKS;
 usb.config=1;native_output_poll();assert(!analog_enabled);
 ticks+=6000000u-1u;native_output_poll();assert(!analog_enabled);
 ticks+=1u;native_output_poll();assert(analog_enabled);
 /* A repeatedly bouncing USB host has a bounded startup delay. */
 native_start();usb.config=0;
 for(unsigned j=0;j<ETCH_OUTPUT_MIN_DMA_HALVES;j++)fm1_alnk0_irq();
 native_output_poll();ticks+=72000000u;
 usb.config=1;native_output_poll();assert(analog_enabled);
 puts("DMA adapter: zero startup, USB gate, priming, stereo Q15 conversion, half selection, held-key panic and deadlines passed");
}
