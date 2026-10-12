#include "../firmware/fm1-native/native_fx.h"
#include "../firmware/fm1-native/wave_editor.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct etch_native_fx f;
static struct etch_wave_editor e;
static int16_t dry[12000],wet[12000];
int main(void){
 struct etch_fx_settings s;etch_fx_defaults(&s);
 etch_fx_reset(&f);
 for(unsigned n=0;n<12000;n++){
  int16_t input=(n<4096)?(int16_t)((n&31u)<16u?9000:-9000):0;
  int16_t l,r;etch_fx_process_stereo_ex(&f,input,0,0,0,&s,&l,&r);
  assert(l==input&&r==input);dry[n]=l;
 }
 etch_fx_reset(&f);
 int64_t changed=0,tail=0,energy=0;
 for(unsigned n=0;n<12000;n++){
  int16_t input=(n<4096)?(int16_t)((n&31u)<16u?9000:-9000):0;
  int16_t l,r;etch_fx_process_stereo_ex(&f,input,0,0,32767,&s,&l,&r);
  wet[n]=l;
  changed+=(l!=dry[n]);
  if(n>4096)tail+=l<0?-l:l;
  energy+=l<0?-l:l;
 }
 assert(changed>1000);
 assert(tail>1000);assert(energy>10000);
 etch_editor_init(&e);
 int16_t a[128],b[128],c[128];
 for(unsigned i=0;i<128;i++){
  e.points[0][i]=(int16_t)((int)i*230-14000);
  e.points[1][i]=(int16_t)(i<64?17000:-17000);
  e.points[2][i]=(int16_t)(i%3*100);
  a[i]=e.points[0][i];b[i]=e.points[1][i];c[i]=e.points[2][i];
 }
 uint32_t rev=e.revision;
 etch_editor_genetics(&e,50,20,17);
 assert(e.wave==2&&e.revision==rev+1&&e.undo_count==1);
 assert(memcmp(e.points[0],a,sizeof a)==0&&memcmp(e.points[1],b,sizeof b)==0);
 assert(memcmp(e.points[2],c,sizeof c)!=0);
 etch_editor_undo(&e);
 assert(memcmp(e.points[2],c,sizeof c)==0);
 etch_editor_genetics(&e,0,0,17);
 assert(memcmp(e.points[2],a,sizeof a)==0);
 etch_editor_genetics(&e,100,0,17);
 assert(memcmp(e.points[2],b,sizeof b)==0);
 puts("V1.14: reverb real wet/dry and non-destructive Wave Genetics undo passed");
}
