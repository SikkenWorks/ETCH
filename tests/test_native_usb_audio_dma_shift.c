/* U02: native USB driver dual-rate packetizer and FIR resampler host test.
 * USB PHY, host enumerator and DAC are not emulated. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FELUCCA_OTA 1
#define FELUCCA_UAC 1
#define FELUCCA_UAC_48K 1
#define FELUCCA_UAC_DMA_SHIFT 7
#define HALF_FRAMES 128u
static void fm1_delay_ms(uint32_t ms){(void)ms;}
static uint32_t ota_now_ms(void){return 0;}
static void ota_idle(void){}
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "../firmware/fm1-native/usb.c"
static int32_t signal_block[HALF_FRAMES*2];
static uint32_t packet[UA_MAXF];
static void exercise(uint32_t rate,uint32_t milliseconds,int32_t drift){
    uac_rate_set(rate);
    assert(!!uac.r48==(rate==48000u));
    uac_stream(1); usb.config=1;usb.up=1;uac.flowing=1;
    uint64_t accumulated=0,received=0;
    uint32_t distinct_stereo=0,nonzero=0;
    for(uint32_t t=0;t<milliseconds;t++){
        accumulated += (uint32_t)(441176+(int32_t)drift);
        while(accumulated >= HALF_FRAMES*10000u){
            accumulated -= HALF_FRAMES*10000u;
            uac_render_start();
            if(uac.feed)uac_tap(signal_block,HALF_FRAMES);
        }
        uint32_t count=uac_packet(packet);
        assert(count >= (rate==48000u ? 47u : 43u));
        assert(count <= (rate==48000u ? 49u : 46u));
        received += count;
        if(t>250u){
            assert(uac.underruns==0 && uac.overruns==0);
            for(uint32_t i=0;i<count;i++){
                if(packet[i])nonzero=1;
                if((uint16_t)packet[i]!=(uint16_t)(packet[i]>>16))distinct_stereo=1;
            }
        }
    }
    assert(received > milliseconds*(rate==48000u?47u:43u));
    assert(uac.pkts>=milliseconds-3u);
    assert(nonzero && distinct_stereo);
    printf("PASS USB %u Hz: packets=%u frames=%u underrun=%u overrun=%u\n",rate,uac.pkts,uac.frames,uac.underruns,uac.overruns);
    uac_stream(0);
}
int main(void){
    for(unsigned i=0;i<HALF_FRAMES;i++){
        signal_block[i*2]=(1200+(int32_t)(i%27))*128;
        signal_block[i*2+1]=(-1900-(int32_t)(i%17))*128;
    }
    /* Direct DMA Q15<<7 conversion must preserve exact 44.1k stereo samples. */
    usb.config=1;uac_stream(1);uac.flowing=1;
    uac_render_start();assert(uac.feed&&uac.go);
    uac_tap(signal_block,HALF_FRAMES);
    assert(ua_ring[UA_PRIME] == ((uint32_t)(uint16_t)1200u |
                ((uint32_t)(uint16_t)(int16_t)-1900 << 16)));
    uac_stream(0);uac_rate_set(44100u);
    const uint8_t *desc;uint16_t len;
    assert(get_desc(0x200,&desc,&len));assert(desc[4]==3u);
    /* Verify that both 44.1/48 sample rates are advertised in the descriptor. */
    uint32_t advertised=0;
    for(uint32_t i=0;i+5<len;i++)if(desc[i]==0x44&&desc[i+1]==0xac&&desc[i+2]==0x00&&
      desc[i+3]==0x80&&desc[i+4]==0xbb&&desc[i+5]==0x00)advertised=1;
    assert(advertised);
    exercise(44100u,12000u,0);exercise(48000u,120000u,44);
    exercise(44100u,12000u,-44);exercise(48000u,120000u,-44);
    exercise(44100u,12000u,0);
    printf("PASS Q15<<7 DMA-equivalent U02 44.1/48/44.1/48/44.1 rate-switch, 2x120s drift soak, stereo and descriptors\n");
}
