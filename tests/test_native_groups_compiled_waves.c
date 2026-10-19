/* Three independent saved waveform triplets, rendered into existing banks. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/wave_editor.h"
#include "../firmware/fm1-native/native_tables.h"
static struct etch_native_bank banks[3];
static int16_t shapes[3][3][128];
static struct etch_wave_fx_params fx[3];
int main(void){
 for(int group=0;group<3;group++)for(int w=0;w<3;w++)for(int n=0;n<128;n++){
   int wave=(group+w)%3;
   shapes[group][w][n]=etch_tables[wave][0][n*4];
 }
 for(int w=0;w<3;w++)fx[w].mix=32767;
 for(int group=0;group<3;group++)etch_editor_compile_source(shapes[group],fx,&banks[group]);
 for(int group=0;group<3;group++)for(int w=0;w<3;w++)for(int l=0;l<6;l++)
  for(int n=0;n<512;n++)assert(banks[group].table[w][l][n]>=-29000 && banks[group].table[w][l][n]<=29000);
 int different=0;
 for(int n=0;n<512;n++)if(banks[0].table[0][0][n]!=banks[1].table[0][0][n])different++;
 assert(different>100);
 for(int n=0;n<512;n++)assert(banks[0].table[0][0][n]==banks[1].table[2][0][n]);
 printf("PASS distinct 3x6x512 bandlimited preset waveforms; differences=%d\n",different);
}
