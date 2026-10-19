/* GPL-3.0-only. Host regression for V1.41 Wave Sculpt. */
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../firmware/fm1-native/native_sculpt.h"
#include "../firmware/fm1-native/wave_editor.h"
int main(void){
 int16_t wave[128],preview[128],raw[128];
 for(int i=0;i<128;i++){wave[i]=(int16_t)((i<64?i:128-i)*850-27000);raw[i]=wave[i];}
 for(int m=0;m<5;m++){
  etch_sculpt_preview(wave,preview,(uint8_t)m,0,50);
  assert(memcmp(wave,preview,sizeof(wave))==0);
  etch_sculpt_preview(wave,preview,(uint8_t)m,26000,80);
  assert(memcmp(wave,raw,sizeof(wave))==0);
  int different=0;for(int i=0;i<128;i++)if(preview[i]!=raw[i])different=1;
  assert(different);
 }
 /* Preview and commit use the same math. Undo restores original 128 samples. */
 struct etch_wave_editor editor={0};
 memcpy(editor.points[0],raw,sizeof(raw));
 etch_editor_sculpt(&editor,3,23500,79);
 assert(editor.undo_count==1u);
 assert(memcmp(editor.points[0],raw,sizeof(raw))!=0);
 etch_editor_undo(&editor);
 assert(memcmp(editor.points[0],raw,sizeof(raw))==0);
 etch_editor_sculpt(&editor,1,0,80);
 assert(editor.undo_count==0u);
 puts("PASS V1.41 Sculpt modes, exact bypass, source preservation, undo");
}
