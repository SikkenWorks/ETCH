/* ETCH V0.72: standalone deterministic OTA animation and live-frame budget. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../firmware/fm1-native/native_ota_matrix.h"
#include "../firmware/fm1-native/native_ui_wave.h"
static uint16_t a[240u*240u],b[240u*240u],slice[240u*40u];
static void render(uint16_t *out,uint32_t frame){
 for(uint32_t i=0;i<6;i++){
  etch_matrix_band(slice,(uint16_t)(i*40u),40u,frame,3u,0);
  memcpy(out+i*240u*40u,slice,sizeof(slice));
 }
}
int main(int argc,char **argv){
 assert(etch_ui_live_frame_interval(8,0,0)==90);
 assert(etch_ui_live_frame_interval(6,0,0)==180);
 assert(etch_ui_live_frame_interval(4,2,0)==350);
 assert(etch_ui_live_frame_interval(4,3,0)==350);
 assert(etch_ui_live_frame_interval(8,0,1)==1000);
 render(a,0);render(b,1);
 uint32_t changed=0,nonnull=0,center=0,outside=0;
 for(uint32_t y=0;y<240;y++)for(uint32_t x=0;x<240;x++){
  uint16_t v=a[y*240+x];
  changed+=(v!=b[y*240+x]);nonnull+=(v!=etch_matrix_pixel(0x0021));
  if(x>=57&&x<185&&y>=104&&y<132 && v==etch_matrix_pixel(0xdfe4))center++;
  if((x<45||x>=195||y<94||y>=143)&&v!=etch_matrix_pixel(0x0021))outside++;
 }
 assert(changed>2000);
 assert(nonnull>5000 && outside>4000);
 assert(center>200);
 uint16_t s=etch_matrix_pixel(0xdfe4);
 assert(a[114*240+59]==s); /* 'E' upper stroke inside centered title */
 printf("PASS V072 OTA animated changed=%u lit=%u title=%u surrounds=%u\n",changed,nonnull,center,outside);
 if(argc==2){FILE *f=fopen(argv[1],"wb");if(!f)return 2;fprintf(f,"P6\n240 240\n255\n");
  for(uint32_t i=0;i<240u*240u;i++){
   uint16_t v=(uint16_t)((a[i]<<8)|(a[i]>>8));
   unsigned char rgb[3]={(unsigned char)((v>>11)*255/31),(unsigned char)(((v>>5)&63)*255/63),(unsigned char)((v&31)*255/31)};
   fwrite(rgb,1,3,f);
  }fclose(f);
 }
 return 0;
}
