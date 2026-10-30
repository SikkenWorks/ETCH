#!/usr/bin/env python3
"""Create two source-referenced alignment previews.
Uses UNMODIFIED V1.82 native-screenshot graph pixels and the V1.83 C bitmap font,
layout helpers and native draw coordinates for changed header/footer regions.
This is NOT a fresh full native LCD execution nor a complete 63-page render.
"""
from pathlib import Path
import zipfile,io,tempfile,subprocess,struct
from PIL import Image
root=Path(__file__).resolve().parents[1]
src=(root/'firmware/fm1-native/bringup.c').read_text()
reference=root/'tests/fixtures/v182_native_graphs'
out=root/'release/V1.83-UI-PREVIEWS';out.mkdir(parents=True,exist_ok=True)

def fn(name):
    import re
    m=re.search(r'(?m)^static [^\n]*\b'+name+r'\s*\(',src)
    if not m:raise ValueError(name)
    a=m.start();b=src.index('{',m.end());depth=1;j=b+1
    while depth:
        if src[j]=='{':depth+=1
        if src[j]=='}':depth-=1
        j+=1
    return src[a:j]

functions='\n'.join(fn(n) for n in ['label','label_box','label_large','etch_label_chars',
   'etch_label_width_px','label_box_centered','label_box_right','label_number_pair','label_version_mode'])
color='''const uint16_t BG=0x0000u,GRID=0x2124u,WHITE=0xf7deu,CYAN=0x67ffu,
 AMBER=0xfde8u,MAGENTA=0xf95bu,LIME=0xdfe4u;'''

host=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "native_phosphor_v182.h"
#include "native_ui_glyph_fast_v173.h"
#define ETCH_UI_VERSION "V183"
static uint16_t pixels[240*240];
static uint8_t native_workflow=1u;
static uint8_t etch_label_scale=1u;
static const uint8_t etch_scope_active=0u;
static uint16_t etch_scope_canvas[240];
static uint16_t etch_scope_origin=0,etch_scope_height=0;
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t color){
 if(x>=240||y>=240)return;
 if(w>240-x)w=240-x;if(h>240-y)h=240-y;
 uint16_t mapped=etch_phosphor_color(color);
 for(uint32_t iy=y;iy<y+h;iy++)for(uint32_t ix=x;ix<x+w;ix++)pixels[iy*240+ix]=mapped;
}
''' +functions+'''
int main(int argc,char **argv){
 if(argc!=5){fprintf(stderr,"args: input output page mode\\n");return 1;}
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 if(fread(pixels,sizeof(pixels),1,f)!=1)return 3;fclose(f);
 native_workflow=(uint8_t)atoi(argv[4]);
 '''+color+'''
 const int morph=atoi(argv[3]);
 lcd_fill(0,0,240,42,BG);
 label_large("ETCH",12,9,LIME);
 label_box(morph?"MORPH":"SOUND",88,14,WHITE,192u);
 label_version_mode(12u,AMBER);
 lcd_fill(12,33,216,1,GRID);
 lcd_fill(0,155,240,85,BG);
 if(morph){
  const uint32_t left_col_l=14u,left_col_r=118u,right_col_l=122u,right_col_r=226u;
  label_number_pair("WAVE A",1u,left_col_l,177u,CYAN,left_col_r);
  label_number_pair("WAVE B",6u,right_col_l,177u,AMBER,right_col_r);
  label_box_centered("K1 A",left_col_l,196u,LIME,66u);
  label_box_centered("K2 B",66u,196u,LIME,118u);
  label_number_pair("K3 MORPH",0u,left_col_l,213u,WHITE,left_col_r);
  label_number_pair("K4 C RATE",1u,right_col_l,213u,MAGENTA,right_col_r);
  label_number_pair("ALG ETCH",3u,left_col_l,229u,MAGENTA,left_col_r);
 }else{
  label("SOUND",14,166,LIME);
  label_box("WARM KEYS",70,166,WHITE,232u);
  label_box(native_workflow?"ADV  SEL: MORPH / FILTER":"SIMPLE  SEL: NEXT  GLO: MODE",14,181,0x7bef,232u);
  const char *names[4]={"MORPH","ETCH","TONE","SPACE"};
  const uint32_t values[4]={0u,3u,7800u,5u};
  for(uint32_t i=0;i<4;i++){
   uint32_t left=4u+i*60u,right=4u+(i+1u)*60u-3u;
   label_box_centered(names[i],left,198,0x7bef,right);
   char digits[11];uint32_t k=10,v=values[i];digits[k]=0;
   do{digits[--k]=(char)('0'+v%10u);v/=10u;}while(v&&k);
   label_box_centered(digits+k,left,214,0xffff,right);
   uint32_t percent=(i==2u)?(values[i]>20000u?100u:values[i]*100u/20000u):
    (values[i]>100u?100u:values[i]);
   lcd_fill(left,233,(percent*54u)/100u,2,i==0?0x67ff:i==1?0xfde8:i==2?0xf95b:0x07e0);
  }
 }
 f=fopen(argv[2],"wb");if(!f)return 4;
 if(fwrite(pixels,sizeof(pixels),1,f)!=1)return 5;fclose(f);
 return 0;
}
'''

def to565(im):
   b=bytearray()
   for r,g,bl in im.getdata():
     code=((r>>3)<<11)|((g>>2)<<5)|(bl>>3)
     b+=struct.pack('<H',code)
   return b

def from565(data):
   vals=struct.unpack('<'+str(240*240)+'H',data)
   rgb=[(((v>>11)&31)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in vals]
   im=Image.new('RGB',(240,240));im.putdata(rgb)
   return im

with tempfile.TemporaryDirectory() as tmp:
  temp=Path(tmp);c=temp/'native_ui_overlay.c';c.write_text(host)
  exe=temp/'native_ui_overlay'
  subprocess.run(['cc','-std=c11','-O2','-w','-I'+str(root/'firmware/fm1-native'),str(c),'-o',str(exe)],check=True)
  z=zipfile.ZipFile('/mnt/data/ETCH-V1.82-ALL-NATIVE-UI-1920x1920.zip')
  for page,idx in [('MORPH','23-MORPH'),('SOUND','00-PLAY')]:
   path='ETCH-V1.82-UI/'+idx+'.png'
   baseline=Image.open(io.BytesIO(z.read(path))).convert('RGB').resize((240,240),Image.Resampling.NEAREST)
   for mode in (0,1):
    inp=temp/'input.raw';dest=temp/'out.raw';inp.write_bytes(to565(baseline))
    subprocess.run([str(exe),str(inp),str(dest),'1' if page=='MORPH' else '0',str(mode)],check=True)
    preview=from565(dest.read_bytes()).resize((1920,1920),Image.Resampling.NEAREST)
    fname=f'ETCH-V1.83-{page}-{("ADVANCED" if mode else "SIMPLE")}-SOURCE-ALIGNED.png'
    preview.save(out/fname,optimize=True)
    print('WROTE',out/fname)
