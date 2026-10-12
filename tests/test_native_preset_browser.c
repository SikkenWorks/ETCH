/* ETCH V1.20 HOME-held PRESETS UI: pure, target-independent regression. */
#include "../firmware/fm1-native/native_preset_browser.h"
#include "../firmware/fm1-native/native_ui_navigation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 struct etch_home_gesture h={0};
 assert(etch_home_gesture_poll(&h,0,0,10)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&h,1,0,100)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&h,1,0,1599)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&h,1,0,1600)==ETCH_HOME_PRESETS);
 assert(etch_home_gesture_poll(&h,1,0,1900)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&h,0,0,2000)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&h,1,0,2100)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&h,0,0,2250)==ETCH_HOME_SHORT);
 assert(etch_home_gesture_poll(&h,0,0,2290)==ETCH_HOME_NONE);
 /* A POWER/recovery + HOME chord must not open the browser or short-nav. */
 assert(etch_home_gesture_poll(&h,1,1,2400)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&h,1,0,3400)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&h,0,0,3500)==ETCH_HOME_NONE);
 /* Timestamp rollover must retain hold detection. */
 assert(etch_home_gesture_poll(&h,1,0,0xfffffe00u)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&h,1,0,0x00000420u)==ETCH_HOME_PRESETS);
 assert(etch_home_gesture_poll(&h,0,0,0x00000430u)==ETCH_HOME_NONE);
 /* Threshold is inclusive, and PRESETS input policy is isolated to its page. */
 struct etch_home_gesture edge={0};
 assert(etch_home_gesture_poll(&edge,1,0,8000)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&edge,1,0,9499)==ETCH_HOME_NONE);
 assert(etch_home_gesture_poll(&edge,1,0,9500)==ETCH_HOME_PRESETS);
 assert(etch_home_gesture_poll(&edge,0,0,9510)==ETCH_HOME_NONE);
 /* Sparse preset banks, plus both wrap directions and large detents. */
 const uint32_t mask=(1u<<0)|(1u<<4)|(1u<<17)|(1u<<31);
 assert(etch_presets_next_valid(0,1,mask)==4);
 assert(etch_presets_next_valid(4,1,mask)==17);
 assert(etch_presets_next_valid(17,1,mask)==31);
 assert(etch_presets_next_valid(31,1,mask)==0);
 assert(etch_presets_next_valid(0,-1,mask)==31);
 assert(etch_presets_next_valid(0,2,mask)==17);
 assert(etch_presets_next_valid(17,-2,mask)==0);
 assert(etch_presets_next_valid(0,0,mask)==0);
 assert(etch_presets_next_valid(4,8,(1u<<4))==4);
 assert(etch_presets_next_valid(4,1,0)==4);
 assert(etch_presets_next_valid(0,-2147483647-1,mask)<32u);
 char name[13];
 char twelve[12]={'B','A','S','S',' ','S','T','R','I','N','G','S'};
 assert(etch_presets_name_copy(name,twelve)==1);
 assert(strcmp(name,"BASS STRINGS")==0);
 char shorter[12]={'L','E','A','D',0,1};
 assert(etch_presets_name_copy(name,shorter)==1);
 assert(strcmp(name,"LEAD        ")==0);
 assert(ETCH_PRESETS_ROW_Y+ETCH_PRESETS_ROWS*ETCH_PRESETS_ROW_H<=213u);
 assert(ETCH_UI_PRESETS==51);
 assert(ETCH_UI_PAGE_COUNT==52);
 assert(strcmp(etch_ui_title(ETCH_UI_PRESETS),"PRESETS")==0);
 puts("V1.20 PRESETS policy tests: PASS");
 return 0;
}
