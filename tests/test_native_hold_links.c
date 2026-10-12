/* ETCH V0.44: deterministic per-trigger HOLD attribution and persistence. */
#include "../firmware/fm1-native/native_project_codec.h"
#include "../firmware/fm1-native/native_seq_ui.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static struct etch_native_pattern p,q;
static struct etch_wave_library w,w2;
static struct etch_project_settings cfg,cfg2;
static uint8_t stream[ETCH_PROJECT_MAX_BYTES];
static struct etch_pattern_bank bank;
int main(void){
 etch_pattern_clear(&p);
 assert(etch_seq_put(&p,0,60,100,1));
 assert(etch_seq_put(&p,1,60,120,1));
 assert(etch_seq_put(&p,1,64,100,2));
 assert(etch_seq_hold_link(&p,16,1,60,0)==1); /* C4 from step 0 continues under a new C4 */
 assert(etch_seq_hold_link(&p,16,2,60,0)==1);
 assert(etch_seq_hold_link(&p,16,3,60,0)==1);
 assert(etch_seq_hold_span(&p,16,0,60)==4);
 assert(etch_seq_hold_span(&p,16,1,60)==1); /* new C4 must NOT inherit old C4 HOLD */
 assert(etch_seq_hold_owner(&p,2,60)==0);
 assert(etch_seq_note_remove(&p,0,0)==2); /* blocked until dependent links removed */
 assert(etch_seq_note_edit(&p,0,0,61,110,2)==3);
 assert(p.steps[0].notes[0].note==60&&p.steps[0].notes[0].length==1);
 assert(etch_seq_hold_link(&p,16,2,60,1)==1); /* two different C4 instances held concurrently */
 assert(etch_seq_hold_owner(&p,2,60)==0);
 assert(etch_seq_hold_span(&p,16,0,60)==4);
 assert(etch_seq_hold_span(&p,16,1,60)==2);
 assert(p.steps[2].hold_count==2&&p.steps[2].holds[0]==60&&p.steps[2].holds[1]==60);
 assert(etch_seq_hold_link(&p,16,2,60,1)==2);
 struct etch_step_layers layers;
 etch_seq_layers(&p,16,2,&layers);
 assert(layers.hold_voices==3&&layers.hold_pitches==2);
 assert(etch_seq_hold_unlink(&p,2,60,0)==1);
 assert(p.steps[2].hold_count==1&&etch_seq_hold_owner(&p,2,60)==1);
 assert(etch_seq_hold_source_cycle(&p,16,2,60,0xffffu,1)==1);
 assert(etch_seq_hold_source_cycle(&p,16,2,60,1,1)==0);
 assert(!etch_seq_hold_link(&p,16,2,60,3)); /* no source trigger */
 assert(!etch_seq_hold_link(&p,16,2,60,2)); /* can't HOLD your own TRIGGER */
 assert(!etch_seq_hold_link(&p,16,17,60,0));
 assert(etch_seq_hold_toggle(&p,1,60)==2);
 assert(etch_seq_hold_toggle(&p,3,60)==2);
 assert(etch_seq_note_remove(&p,0,0)==1); /* no surviving references */
 assert(etch_seq_note_edit(&p,1,0,61,110,2)==3); /* step2 still depends on new C4 */
 /* Test a wrap from step 255 to step 0, full 256 length. */
 etch_pattern_clear(&p);assert(etch_seq_put(&p,255,70,127,1));
 assert(etch_seq_put(&p,0,70,127,1));
 assert(etch_seq_hold_link(&p,256,0,70,255)==1);
 assert(etch_seq_hold_link(&p,256,1,70,255)==1);
 assert(etch_seq_hold_span(&p,256,255,70)==3);
 assert(etch_seq_hold_span(&p,256,0,70)==1);
 assert(etch_seq_hold_owner(&p,0,70)==255);
 assert(p.steps[0].hold_ago[0]==1&&p.steps[1].hold_ago[0]==2);
 /* Short loop wraps from step15 to step0; offset is absolute step identity. */
 etch_pattern_clear(&p);assert(etch_seq_put(&p,15,67,127,1));
 assert(etch_seq_put(&p,0,67,95,1));
 assert(etch_seq_hold_link(&p,16,0,67,15)==1);
 assert(etch_seq_hold_link(&p,16,1,67,15)==1);
 assert(p.steps[0].hold_ago[0]==241&&p.steps[1].hold_ago[0]==242);
 assert(etch_seq_hold_owner(&p,0,67)==15);
 assert(etch_seq_hold_span(&p,16,15,67)==3);
 assert(etch_seq_hold_span(&p,16,0,67)==1);
 /* Individual linked HOLD instances pass through per-patch and V8 project codecs. */
 assert(etch_pattern_bank_capture(&bank,5,&p)==0);
 assert(etch_pattern_bank_validate(&bank)==0);
 assert(etch_pattern_bank_fetch(&bank,5,&q)==1);
 assert(memcmp(&p,&q,sizeof(p))==0);
 cfg.seq_length=16;cfg.tempo=120;cfg.seq_gate=75;etch_fx_defaults(&cfg.global_fx);
 cfg.patterns=bank;
 unsigned used=0;
 assert(etch_project_encode(&w,&p,&cfg,stream,sizeof(stream),&used)==0);
 assert(memcmp(stream,"PJT<",4)==0);
 assert(etch_project_decode(stream,used,&w2,&q,&cfg2)==0);
 assert(cfg2.source_version==12);
 assert(memcmp(&p,&q,sizeof(p))==0);
 assert(etch_pattern_bank_fetch(&cfg2.patterns,5,&q)==1);
 assert(memcmp(&p,&q,sizeof(p))==0);
 /* tamper link ownership with no matching source; must reject rather than load */
 p.steps[0].hold_ago[0]=7;
 assert(etch_project_encode(&w,&p,&cfg,stream,sizeof(stream),&used)==-1);
 assert(etch_pattern_bank_capture(&bank,5,&p)==-1);
 /* Legacy genuine PJT7 fixture is loaded without altering pitch-level HOLD data. */
 FILE *f=fopen("tests/fixtures/native-project-v7.bin","rb");assert(f);
 unsigned n=(unsigned)fread(stream,1,sizeof(stream),f);fclose(f);
 assert(n>100&&memcmp(stream,"PJT7",4)==0);
 assert(etch_project_decode(stream,n,&w2,&q,&cfg2)==0);
 assert(cfg2.source_version==7);
 assert(q.steps[0].hold_count==1&&q.steps[0].hold_ago[0]==0);
 assert(cfg2.patterns.used>0);
 assert(etch_pattern_bank_fetch(&cfg2.patterns,0,&p)==1);
 assert(p.steps[1].hold_count==1&&p.steps[1].hold_ago[0]==0);
 used=0;assert(etch_project_encode(&w2,&q,&cfg2,stream,sizeof(stream),&used)==0);
 assert(memcmp(stream,"PJT<",4)==0);
 assert(etch_project_decode(stream,used,&w,&p,&cfg)==0&&cfg.source_version==12);
 assert(p.steps[0].hold_ago[0]==0);
 puts("PASS V0.44 HOLD owner attribution, double-trigger, 16/256 wrap, edit/delete guards, V7->V8 migration, per-patch and project roundtrip");
}
