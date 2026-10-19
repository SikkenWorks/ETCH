#include "../firmware/fm1-native/native_group_router.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct etch_key_groups groups;etch_key_groups_init(&groups);
 struct etch_group_capture c={0};
 assert(etch_key_group_route(&groups,60)==0);
 etch_group_capture_begin(&c,0);
 assert(etch_group_capture_note(&c,&groups,48,4)==0);
 assert(etch_group_capture_note(&c,&groups,48,4)==2);
 assert(etch_group_capture_note(&c,&groups,47,4)==2);
 assert(etch_group_capture_note(&c,&groups,59,4)==1);
 assert(etch_key_group_route(&groups,48)==1&&etch_key_group_route(&groups,59)==1);
 assert(etch_key_group_route(&groups,60)==0);
 etch_group_capture_begin(&c,1);
 assert(etch_group_capture_note(&c,&groups,55,7)==0);
 assert(etch_group_capture_note(&c,&groups,64,7)==2); /* overlap */
 etch_group_capture_begin(&c,1);
 assert(etch_group_capture_note(&c,&groups,60,7)==0);
 assert(etch_group_capture_note(&c,&groups,72,7)==1);
 assert(etch_key_group_route(&groups,60)==2);
 uint8_t data[8]={0};etch_key_groups_pack(&groups,data);
 struct etch_key_groups restored;etch_key_groups_init(&restored);
 assert(etch_key_groups_unpack(&restored,data));
 assert(etch_key_group_route(&restored,60)==2);
 data[5]=255;assert(!etch_key_groups_unpack(&restored,data));
 assert(etch_key_group_route(&restored,60)==2); /* no mutation on failure */
 assert(etch_key_group_set(&groups,0,4,48,59,0));
 assert(etch_key_group_route(&groups,55)==0);
 etch_group_capture_begin(&c,0);etch_group_capture_cancel(&c);
 assert(etch_group_capture_note(&c,&groups,36,2)==3);
 puts("native GROUPS capture / routing / serialization tests passed");
 return 0;
}
