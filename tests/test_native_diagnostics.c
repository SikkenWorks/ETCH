#include "../firmware/fm1-native/native_diagnostics.h"
#include "../firmware/fm1-native/native_ui_navigation.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct etch_diag_panel d={0};int32_t en[7]={2,-3,4,-5,0,11,-1};
 etch_diag_panel_record(&d,en,0x5u,0x21u);
 assert(d.turns[0]==2&&d.turns[1]==3&&d.turns[2]==4&&d.turns[3]==5&&d.turns[5]==11&&d.turns[6]==1);
 assert(etch_diag_count(d.key_seen)==2&&etch_diag_count(d.button_seen)==2);
 assert(etch_diag_cpu_permille(544,5440)==100);
 assert(etch_diag_cpu_permille(9999,0)==0);
 assert(etch_diag_abspeak(-32768)==32768);
 for(int i=0;i<7;i++)assert(etch_ui_system_tab(i,1)==((i+1)%7));
 for(int i=0;i<7;i++)assert(etch_ui_system_tab(i,-1)==((i+6)%7));
 d.turns[0]=UINT32_MAX;etch_diag_panel_record(&d,en,0x2u,0x4u);
 assert(d.turns[0]==UINT32_MAX&&etch_diag_count(d.key_seen)==3);
 assert(d.clockwise==((1u<<0)|(1u<<2)|(1u<<5)));
 assert(d.anticlockwise==((1u<<1)|(1u<<3)|(1u<<6)));
 assert(etch_diag_panel_missing_keys(&d)==24);
 assert(etch_diag_panel_missing_buttons(&d)==11);
 assert(etch_diag_panel_missing_encoder_directions(&d)==8);
 assert(!etch_diag_panel_complete(&d));
 struct etch_diag_panel complete={0};
 int32_t cw[7]={1,1,1,1,1,1,1},ccw[7]={-1,-1,-1,-1,-1,-1,-1};
 for(unsigned k=0;k<27;k++)etch_diag_panel_record(&complete,cw,1u<<k,0);
 for(unsigned b=0;b<14;b++)etch_diag_panel_record(&complete,ccw,0,1u<<b);
 assert(etch_diag_panel_complete(&complete));
 assert(etch_diag_panel_missing_keys(&complete)==0);
 assert(etch_diag_panel_missing_buttons(&complete)==0);
 assert(etch_diag_panel_missing_encoder_directions(&complete)==0);
 complete.anticlockwise &= (uint8_t)~(1u<<6);
 assert(!etch_diag_panel_complete(&complete));
 puts("PASS V0.53 read-only panel diagnostics, callback CPU budget math, seven SYSTEM tabs");
}
