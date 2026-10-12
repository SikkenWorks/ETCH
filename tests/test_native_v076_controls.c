#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "../firmware/fm1-native/native_save_controls.h"
#include "../firmware/fm1-native/native_led_policy.h"
#include "../firmware/fm1-native/native_ui_scope_policy.h"
#include "../firmware/fm1-native/native_fx_controls.h"
int main(void){
 char name[13]="TEST        ";uint8_t pos=4;
 assert(etch_save_position(0,-1)==0);
 assert(!etch_save_has_name("            "));
 assert(etch_save_has_name("C TEST      "));
 assert(etch_save_position(11,5)==11);
 assert(etch_save_position(0,5)==5);
 etch_save_backspace(name,&pos,-1);assert(pos==3 && name[3]==' ');
 etch_save_backspace(name,&pos,1);assert(pos==3);
 assert(etch_save_toast_visible(150,200));assert(!etch_save_toast_visible(250,200));
 for(uint8_t k=2;k<14;k++)assert(etch_led_function(k));
 assert(!etch_led_function(1));
 assert(etch_led_octave(0,-2,0) && !etch_led_octave(1,-2,0));
 assert(etch_led_octave(1,1,0) && !etch_led_octave(0,1,0));
 assert(etch_scope_redraw_on_silence(0,1));assert(!etch_scope_redraw_on_silence(0,0));
 assert(etch_scope_release_gain(1024,0,5000)==0);
 assert(etch_scope_release_gain(1024,2500,5000)<1024);
 assert(etch_cutoff_turn(8192,-5)==8142);
 assert(etch_cutoff_turn(500,5)==550);
 assert(etch_filter_enable_if_cutoff_edited(0,-1)==1);
 assert(etch_filter_enable_if_cutoff_edited(0,0)==0);
 puts("V0.76 SAVE/LED/SCOPE/FILTER control tests: PASS");return 0;
}
