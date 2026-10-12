/* V0.42 authored 16-step ARP: selected-key order, rests, length and invalid data */
#include "../firmware/fm1-native/native_performance.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct etch_arp_program p={0};
 assert(etch_arp_program_valid(&p));
 assert(etch_arp_program_pick(&p,0,3)==0);
 assert(etch_arp_program_pick(&p,4,3)==1);
 etch_arp_program_init(&p);
 assert(p.length==16&&p.steps[0]==0&&p.steps[15]==15);
 p.length=4;p.steps[0]=2;p.steps[1]=255;p.steps[2]=0;p.steps[3]=3;
 assert(etch_arp_program_valid(&p));
 assert(etch_arp_program_pick(&p,0,3)==2);
 assert(etch_arp_program_pick(&p,1,3)==255);
 assert(etch_arp_program_pick(&p,2,3)==0);
 assert(etch_arp_program_pick(&p,3,3)==0);
 assert(etch_arp_program_pick(&p,4,3)==2);
 assert(etch_arp_program_pick(&p,0,0)==255);
 p.steps[0]=254;assert(!etch_arp_program_valid(&p));
 assert(etch_arp_program_pick(&p,0,3)==0);
 p.steps[0]=3;p.length=17;assert(!etch_arp_program_valid(&p));
 puts("PASS custom 16-step ARP order/rests/length/clamping and validation");
 return 0;
}
