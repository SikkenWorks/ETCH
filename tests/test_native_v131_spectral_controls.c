/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_spectral_controls_v131.h"
static struct etch_wave_editor e;
int main(void){
 etch_editor_init(&e);e.wave=1;
 uint8_t gain=50u,tilt=50u;
 etch_spectral_controls_turn(&e,4,&gain,&tilt,+5,0);
 assert(gain==55u && tilt==50u);
 uint32_t r=e.revision;
 etch_spectral_controls_turn(&e,4,&gain,&tilt,0,-3);
 assert(gain==55u && tilt==47u && e.revision>r);
 etch_spectral_controls_turn(&e,4,&gain,&tilt,-5,0);
 assert(gain==50u && tilt==47u);
 etch_spectral_controls_turn(&e,4,&gain,&tilt,-150,+150);
 assert(gain==0u && tilt==100u);
 r=e.revision;
 etch_spectral_controls_turn(&e,4,&gain,&tilt,-10,+10);
 assert(gain==0u && tilt==100u && e.revision==r);
 puts("PASS SPECTRAL K3/K4 independent positions and correct bounds");
}
