/* SPDX-License-Identifier: GPL-3.0-only */
#include "../firmware/fm1-native/native_panel_midi.h"
#include <assert.h>
#include <stdio.h>
static uint32_t packets[200];static int count,capacity=200;
static int emit(uint32_t p,void *context){(void)context;if(count==capacity)return 0;packets[count++]=p;return 1;}
static void clear_packets(void){count=0;capacity=200;}
int main(void){
 struct etch_panel_midi m={0};etch_panel_midi_reset(&m);
 etch_panel_midi_update(&m,1u,0,0,emit,0);assert(count==0);
 etch_panel_midi_update(&m,1u,0,1,emit,0);assert(count==1&&packets[0]==(0x7f359009u));
 etch_panel_midi_update(&m,1u,0,1,emit,0);assert(count==1); /* no duplicate */
 clear_packets();etch_panel_midi_update(&m,1u,1,1,emit,0);
 assert(count==2&&packets[0]==0x00358008u&&packets[1]==0x7f419009u); /* release THEN 12 semitones up */
 clear_packets();capacity=0;etch_panel_midi_update(&m,0u,1,1,emit,0);assert(m.active[0]==1);
 capacity=200;etch_panel_midi_update(&m,0u,1,1,emit,0);assert(count==1&&packets[0]==0x00418008u);
 clear_packets();etch_panel_midi_update(&m,3u,0,1,emit,0);assert(count==2);
 etch_panel_midi_update(&m,0u,0,0,emit,0);assert(m.active[0]==0&&m.active[1]==0); /* USB disconnect */
 clear_packets();etch_panel_midi_update(&m,1u,-4,1,emit,0);assert(count==1&&packets[0]==0x7f059009u);
 puts("FM1 USB MIDI OUT: note on/off, octave retune, backpressure, disconnect, low octave PASS");
}
