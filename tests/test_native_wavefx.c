#include "../firmware/fm1-native/wave_editor.h"
#include <assert.h>
#include <stdio.h>
static struct etch_wave_editor e;
static int16_t in[128],out[128];
int main(void){
 for(int i=0;i<128;i++)in[i]=i<64?29000:-29000;
 struct etch_wave_fx_params fx={0};fx.mix=32767;
 etch_wave_fx_apply(in,&fx,out);for(int i=0;i<128;i++)assert(in[i]==out[i]);
 fx.drive=24000;fx.bitcrush=27000;fx.chorus=16000;fx.delay=24000;
 fx.reverb=22000;fx.fold=32000;fx.smooth=21000;
 etch_wave_fx_apply(in,&fx,out);
 int different=0;for(int i=0;i<128;i++){different+=(out[i]!=in[i]);assert(out[i]>=-29000&&out[i]<=29000);}
 assert(different>40);
 etch_editor_init(&e);e.fx[0]=fx;
 etch_editor_bake_fx(&e);assert(e.fx[0].drive==0&&e.fx[0].mix==32767&&e.dirty);
 puts("Native WAVE FX: dry identity, seven cyclic processors, bounded output and explicit bake passed");
}
