#include "../firmware/fm1-native/wave_editor.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static struct etch_wave_editor e;
int main(void){
 etch_editor_init(&e);for(unsigned i=0;i<128;i++)e.points[0][i]=16384;
 etch_editor_tool(&e,4,0);for(unsigned i=0;i<128;i++)assert(e.points[0][i]==0);
 etch_editor_tool(&e,10,0);assert(e.points[0][0]==29000&&e.points[0][64]==-29000);
 etch_editor_tool(&e,3,0);assert(e.points[0][0]==-29000);
 etch_editor_tool(&e,2,0);assert(e.points[0][127]==e.points[0][0]);
 etch_editor_tool(&e,9,0);assert(e.points[0][0]<e.points[0][127]);
 etch_editor_tool(&e,11,64);assert(e.points[0][0]>e.points[0][127]);
 etch_editor_tool(&e,5,0);for(unsigned i=0;i<128;i++)assert(e.points[0][i]==0);
 etch_editor_tool(&e,6,0);long energy=0;for(unsigned i=0;i<128;i++)energy+=abs(e.points[0][i]);assert(energy>100000);
 etch_editor_tool(&e,1,0);assert(e.dirty&&e.undo_count>0);
 puts("Native draw tools: normalization, square, invert, mirror, saw, rotation, flatten, random and smooth passed");
}
