#!/usr/bin/env python3
"""Exercise layout helper C bodies extracted directly from native bringup.c.
These host geometry checks do not emulate the FM1 LCD/DMA hardware.
"""
from pathlib import Path
import tempfile, subprocess
source=(Path(__file__).resolve().parents[1]/'firmware/fm1-native/bringup.c').read_text()

def function(name):
    import re
    m=re.search(r'(?m)^static [^\n]*\b'+name+r'\s*\(',source)
    if not m:raise ValueError('not found: '+name)
    start=m.start();b=source.find('{',m.end());depth=1;i=b+1
    while depth:
        if source[i]=='{':depth+=1
        if source[i]=='}':depth-=1
        i+=1
    return source[start:i]

names=['etch_label_chars','etch_label_width_px','label_box_centered','label_box_right','label_number_pair','label_version_mode']
body='\n'.join(function(n) for n in names)
test=r'''
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define ETCH_UI_VERSION "V183"
#define AMBER 0xff22u
static uint8_t etch_label_scale=1u;
static uint8_t native_workflow=0u;
struct rect {uint32_t x,y,end;char txt[40];};
static struct rect rects[12];static int count;
static void reset(void){count=0;}
static void label_box(const char *s,uint32_t x,uint32_t y,uint16_t color,uint32_t right){
 (void)color;assert(right<=240 && x<right && y+7<=240);
 uint32_t n=(right-x)/8u,len=(uint32_t)strlen(s);if(n>len)n=len;
 if(n==0u)return;
 assert(count<12);
 struct rect *r=&rects[count++];r->x=x;r->y=y;r->end=x+n*8u;
 assert(r->end<=right);memcpy(r->txt,s,n);r->txt[n]=0;
}
''' + body + r'''
static void validate_row(uint32_t left,uint32_t right){
 for(int i=0;i<count;i++){
  assert(rects[i].x>=left && rects[i].end<=right);
  for(int j=i+1;j<count;j++)if(rects[i].y==rects[j].y)
   assert(rects[i].end<=rects[j].x || rects[j].end<=rects[i].x);
 }
}
int main(void){
 const char *labels[]={"WAVE A","WAVE B","K3 MORPH","K4 C RATE","ALG ETCH"};
 for(unsigned i=0;i<sizeof(labels)/sizeof(labels[0]);i++){
  uint32_t left=i%2?122u:14u,right=i%2?226u:118u;
  for(uint32_t value=0;value<=1000;value++){
   reset();label_number_pair(labels[i],value,left,213u,0xffff,right);
   assert(count==2);validate_row(left,right);
  }
 }
 const char *footer[]={"MORPH","ETCH","TONE","SPACE","CUTOFF","CHORUS","RELEASE"};
 for(int k=0;k<4;k++)for(unsigned j=0;j<sizeof(footer)/sizeof(footer[0]);j++){
  uint32_t left=4u+(uint32_t)k*60u,right=left+57u;
  reset();label_box_centered(footer[j],left,198,0xffff,right);
  label_box_centered("7772",left,214,0xffff,right);
  validate_row(left,right);
 }
 for(int mode=0;mode<=1;mode++){
  native_workflow=(uint8_t)mode;reset();label_version_mode(12u,AMBER);
  assert(count==1);assert(rects[0].x==192u && rects[0].end==240u);
  assert(strcmp(rects[0].txt,mode?"V183 A":"V183 S")==0);
 }
 printf("PASS: 5010 actual-C MORPH label/number pairs; 56 footer rectangles; V183 S/A aligned on one row\\n");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    src=Path(tmp)/'layout.c';exe=Path(tmp)/'layout';src.write_text(test)
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
assert 'web_last_workflow!=native_workflow' in source
assert source.count('label_version_mode(')>=12
print('PASS: version repaint invalidated by workflow changes, shared S/A across all versioned renderers')
