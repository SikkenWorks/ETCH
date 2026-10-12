#include <assert.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_panel_capture.h"
int main(void){
 assert(etch_panel_capture_action(0,0,1u<<13,0)==ETCH_PANEL_NORMAL);
 assert(etch_panel_capture_action(0,1,1u<<13,0)==ETCH_PANEL_BEGIN);
 assert(etch_panel_capture_action(0,1,1u<<9,0)==ETCH_PANEL_NORMAL);
 /* During capture every single button, including REC, OCT+/- and HOME,
  * is data, not an application command. */
 for(unsigned b=0;b<14;b++)assert(etch_panel_capture_action(1,1,1u<<b,1u<<b)==ETCH_PANEL_CAPTURE);
 assert(etch_panel_capture_action(1,1,3u,3u)==ETCH_PANEL_CAPTURE);
 assert(etch_panel_capture_action(1,0,0,0)==ETCH_PANEL_CAPTURE);
 assert(etch_panel_capture_action(1,1,(1u<<8)|(1u<<9),(1u<<8)|(1u<<9))==ETCH_PANEL_END);
 puts("PASS I02 exclusive diagnostic capture: 14 buttons, exit chord, no octave/boot side effects");
}
