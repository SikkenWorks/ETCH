#include "../firmware/fm1-native/wave_editor.h"
#include <stdio.h>
#include <time.h>
static struct etch_wave_editor e;
static struct etch_native_bank b,c;
int main(void){
 etch_editor_init(&e);
 volatile int check=0;
 clock_t start=clock();
 for(int i=0;i<8;i++){etch_editor_compile(&e,&b);check+=b.table[0][0][i];}
 clock_t full=clock()-start;
 start=clock();
 for(int i=0;i<8;i++){
  etch_editor_compile_wave_source(e.points,e.fx,&c,0);
  for(int w=1;w<3;w++)for(int l=0;l<6;l++)for(int n=0;n<512;n++)c.table[w][l][n]=b.table[w][l][n];
  check+=c.table[0][0][i];
 }
 clock_t part=clock()-start;
 printf("HOST BENCH 8 full: %.3f s; 8 partial+copy: %.3f s; ratio %.2f (not FM1 timings) %d\n",(double)full/CLOCKS_PER_SEC,(double)part/CLOCKS_PER_SEC,(double)full/part,check);
}
