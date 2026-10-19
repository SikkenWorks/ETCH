#include <assert.h>
#include "../firmware/fm1-native/native_group_router.h"
int main(void){
 struct etch_key_groups g={{{1,1,48,59},{1,2,60,72}}};
 assert(etch_key_groups_valid(&g));
 assert(etch_key_group_route(&g,47)==0);
 assert(etch_key_group_route(&g,48)==1);
 assert(etch_key_group_route(&g,59)==1);
 assert(etch_key_group_route(&g,60)==2);
 assert(etch_key_group_route(&g,72)==2);
 assert(etch_key_group_route(&g,73)==0);
 g.group[1].start=59;assert(!etch_key_groups_valid(&g));
 g.group[1].enabled=0;assert(etch_key_groups_valid(&g));
 g.group[0].start=60;g.group[0].end=60;assert(!etch_key_groups_valid(&g));
 return 0;
}
