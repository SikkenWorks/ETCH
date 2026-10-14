/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_advanced_controls_v130.h"
int main(void){
 struct etch_organic_settings o={20,30,40,50};
 etch_v130_organic(&o,0,1,0,0);
 assert(o.drift==20&&o.rate==31&&o.phase==40&&o.variance==50);
 etch_v130_organic(&o,1,0,0,0);assert(o.drift==21&&o.rate==31);
 etch_v130_organic(&o,0,0,-2,0);assert(o.phase==38&&o.variance==50);
 etch_v130_organic(&o,0,0,0,5);assert(o.variance==55);
 struct etch_living_settings l={60,40,20,10};
 etch_v130_living(&l,0,1,0,0);assert(l.depth==60&&l.rate==41&&l.color==20&&l.vary==10);
 etch_v130_living(&l,0,0,1,0);assert(l.depth==60&&l.rate==41&&l.color==21);
 etch_v130_living(&l,0,0,0,-3);assert(l.vary==7&&l.depth==60);
 etch_v130_living(&l,-3,0,0,0);assert(l.depth==57&&l.rate==41);
 etch_v130_living(&l,-999,999,0,0);assert(l.depth==0&&l.rate==100);
 struct etch_mod_matrix m={0};uint8_t slot=0;
 m.route[0]=(struct etch_mod_route){1,2,50};
 etch_v130_matrix(&m,&slot,0,0,0,4);
 assert(slot==0&&m.route[0].source==1&&m.route[0].target==2&&m.route[0].amount==54);
 etch_v130_matrix(&m,&slot,0,1,0,0);assert(m.route[0].source==2&&m.route[0].target==2);
 etch_v130_matrix(&m,&slot,0,0,-1,0);assert(m.route[0].target==1&&m.route[0].amount==54);
 etch_v130_matrix(&m,&slot,1,0,0,0);assert(slot==1&&m.route[0].amount==54&&m.route[1].amount==0);
 assert(etch_v130_bpm(120,4)==124);
 assert(etch_v130_bpm(240,40)==240&&etch_v130_bpm(30,-40)==30);
 puts("PASS V1.30 independent controls and BPM 30..240");
}
