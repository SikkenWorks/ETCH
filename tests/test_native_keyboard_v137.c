/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/native_keyboard_boot.h"
#include "../firmware/fm1-native/native_panel_midi.h"
#include <assert.h>
#include <stdio.h>
static uint32_t sent[256];static int count,room=256;
static int emit(uint32_t pkt,void *ignored){(void)ignored;if(count>=room)return 0;sent[count++]=pkt;return 1;}
static void clear(void){count=0;room=256;}
int main(void){
 assert(etch_keyboard_boot_requested(0,ETCH_BUTTON_SEQ));
 assert(!etch_keyboard_boot_requested(1,ETCH_BUTTON_SEQ)); /* REC / recovery */
 assert(!etch_keyboard_boot_requested(0,0));
 assert(!etch_keyboard_boot_requested(0,ETCH_BUTTON_SEQ|ETCH_BUTTON_OCT_DOWN)); /* ROM loader */
 assert(etch_keyboard_octave_step(0,ETCH_BUTTON_OCT_UP)==1);
 assert(etch_keyboard_octave_step(0,ETCH_BUTTON_OCT_DOWN)==-1);
 assert(etch_keyboard_octave_step(0,3u)==0);
 assert(etch_keyboard_octave_step(3,ETCH_BUTTON_OCT_UP)==3);
 assert(etch_keyboard_octave_step(-3,ETCH_BUTTON_OCT_DOWN)==-3);
 struct etch_panel_midi k={0};etch_panel_midi_reset(&k);
 etch_panel_midi_update(&k,(1u<<0)|(1u<<12)|(1u<<26),0,1,emit,0);
 assert(count==3);
 assert(sent[0]==0x7f359009u); /* F3 / MIDI 53 */
 assert(sent[1]==0x7f419009u); /* F4 / MIDI 65 */
 assert(sent[2]==0x7f4f9009u); /* G5 / MIDI 79 */
 clear();etch_panel_midi_update(&k,(1u<<0)|(1u<<12),1,1,emit,0);
 assert(count==5); /* Three note-offs BEFORE two newly transposed note-ons */
 for(int i=0;i<3;i++)assert((sent[i]&0xffu)==0x08u);
 for(int i=3;i<5;i++)assert((sent[i]&0xffu)==0x09u);
 clear();room=0;etch_panel_midi_update(&k,0u,1,1,emit,0);
 assert(k.active[0]&&k.active[12]); /* pending off not lost on USB queue backpressure */
 room=256;etch_panel_midi_update(&k,0u,1,1,emit,0);
 assert(count==2&&!k.active[0]&&!k.active[12]);
 clear();etch_panel_midi_update(&k,1u,0,1,emit,0);assert(count==1);
 etch_panel_midi_update(&k,1u,0,0,emit,0); /* loss of host clears local voices */
 clear();etch_panel_midi_update(&k,0u,0,1,emit,0);assert(count==0);
 puts("PASS V1.37 SEQ boot priority, note-on/off, chords, octave retuning, USB disconnect/backpressure");
 return 0;
}
