/* SPDX-License-Identifier: GPL-3.0-only
 * ETCH host-side native AUDIO test runner (not a complete FM-1 hardware emulator).
 * Compiles the EXACT audio_core.c / native_fx.c / wave_editor.c used by the target.
 * Usage: etch_host SCENARIO.etch OUTPUT.wav RESULT.json
 */
#include "../firmware/fm1-native/audio_core.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#define SAMPLE_RATE 44100u
#define BUFFER_SIZE 128u
static struct etch_native_audio state;
static struct etch_native_controls controls;
static struct etch_wave_editor editor;
static struct etch_native_bank compiled_bank;
static struct etch_native_pattern pattern;
static uint32_t keys_down;
static FILE *wav;
static uint64_t frames_written, abs_total, frames_since_check, abs_since_check, clips;
static uint32_t peak, assertion_count, commands, note_events, max_voices, perf_events;
static uint32_t last_perf_note;
static int have_perf_note;

static void fail(int line,const char *message){fprintf(stderr,"ETCH host: line %d: %s\n",line,message);exit(2);}
static void require(int line,int yes,const char *message){assertion_count++;if(!yes)fail(line,message);}
static void write16(FILE *f,uint16_t val){fputc((int)(val&255),f);fputc((int)(val>>8),f);}
static void write32(FILE *f,uint32_t val){write16(f,(uint16_t)val);write16(f,(uint16_t)(val>>16));}
static void wav_begin(FILE *f){
 fwrite("RIFF",1,4,f);write32(f,0);fwrite("WAVEfmt ",1,8,f);
 write32(f,16);write16(f,1);write16(f,1);write32(f,SAMPLE_RATE);
 write32(f,SAMPLE_RATE*2);write16(f,2);write16(f,16);
 fwrite("data",1,4,f);write32(f,0);
}
static void wav_end(FILE *f){
 if(frames_written>0x7fffffd0u){fprintf(stderr,"WAV too big\n");exit(3);}
 if(fseek(f,4,SEEK_SET)||ferror(f))exit(3);
 write32(f,(uint32_t)(36+frames_written*2));
 if(fseek(f,40,SEEK_SET)||ferror(f))exit(3);
 write32(f,(uint32_t)(frames_written*2));
 if(fclose(f))exit(3);
}
static uint32_t number(int line,const char *s){
 if(!s)fail(line,"missing argument");
 errno=0;char *end=0;unsigned long v=strtoul(s,&end,0);
 if(errno||end==s||*end||v>UINT32_MAX)fail(line,"invalid unsigned integer");
 return (uint32_t)v;
}
static uint32_t bounded(int line,const char *s,uint32_t max){uint32_t v=number(line,s);if(v>max)fail(line,"parameter out of range");return v;}
static void reset(void){
 etch_native_reset(&state);etch_editor_init(&editor);etch_pattern_clear(&pattern);
 memset(&controls,0,sizeof(controls));controls.gain=30000;controls.morph=8192;
 controls.tempo=120;controls.arp_octaves=1;controls.seq_length=16;controls.seq_gate=75;
 controls.attack_step=0x7fff0000;controls.decay_step=0x7fff0000;
 controls.release_step=0x00600000;controls.sustain_level=28000;
 for(int i=0;i<64;i++)controls.seq_notes[i]=255;
 keys_down=0;have_perf_note=0;last_perf_note=0;
}
static void observe_voices(void){
 uint32_t count=0;
 for(unsigned i=0;i<ETCH_NATIVE_VOICES;i++){
  struct etch_native_voice *v=&state.voice[i];
  if(v->active)count++;
  if(v->active&&v->held&&v->note>=4096u&&(!have_perf_note||last_perf_note!=v->note)){
   last_perf_note=v->note;have_perf_note=1;perf_events++;
  }
 }
 if(count>max_voices)max_voices=count;
}
static void render(uint32_t total){
 int16_t block[BUFFER_SIZE];
 while(total){
  uint32_t n=total>BUFFER_SIZE?BUFFER_SIZE:total;
  etch_native_render(&state,&controls,keys_down,block,n);
  observe_voices();
  for(uint32_t i=0;i<n;i++){
   int32_t v=block[i];uint32_t magnitude=(uint32_t)(v<0?-v:v);
   if(magnitude>peak)peak=magnitude;
   if(magnitude>=32767u)clips++;
   abs_total+=magnitude;abs_since_check+=magnitude;
   write16(wav,(uint16_t)block[i]);
  }
  frames_written+=n;frames_since_check+=n;total-=n;
 }
}
static void set_param(int line,const char *name,const char *value){
 uint32_t v=number(line,value);
 #define SET(x, max) if(!strcmp(name,#x)){if(v>(max))fail(line,"parameter out of range: " #x);controls.x=(uint16_t)v;return;}
 SET(morph,32767u) SET(depth,32767u) SET(gain,32767u)
 SET(sustain_level,32767u) SET(cutoff,8192u) SET(resonance,32767u)
 SET(drive,32767u) SET(chorus,32767u) SET(delay,32767u) SET(reverb,32767u)
 #undef SET
 #define SET8(x, max) if(!strcmp(name,#x)){if(v>(max))fail(line,"parameter out of range: " #x);controls.x=(uint8_t)v;return;}
 SET8(mod_target,1u) SET8(mod_mode,2u) SET8(mod_ratio,11u) SET8(arp_mode,5u)
 SET8(arp_rate,3u) SET8(arp_octaves,4u) SET8(seq_run,1u)
 SET8(seq_rate,3u) SET8(seq_gate,100u) SET8(filter_type,3u) SET8(env_enabled,1u)
 SET8(tempo,255u) SET8(seq_swing,75u) SET8(clock_follow,1u)
 #undef SET8
 if(!strcmp(name,"octave")){if(v>8)fail(line,"octave must be 0..8 in host scene");controls.octave=(int8_t)v;return;}
 if(!strcmp(name,"seq_length")){if(v!=8&&v!=16&&v!=32&&v!=64&&v!=128&&v!=256)fail(line,"sequence length must be 8/16/32/64/128/256");controls.seq_length=(uint16_t)v;return;}
 if(!strcmp(name,"root")){controls.harmony.root=(uint8_t)bounded(line,value,11);return;}
 if(!strcmp(name,"mode")){controls.harmony.mode=(uint8_t)bounded(line,value,8);return;}
 if(!strcmp(name,"keylock")){controls.harmony.keylock=(uint8_t)bounded(line,value,2);return;}
 if(!strcmp(name,"chord")){controls.harmony.chord=(uint8_t)bounded(line,value,2);return;}
 if(!strcmp(name,"attack_step")){controls.attack_step=v;return;}
 if(!strcmp(name,"decay_step")){controls.decay_step=v;return;}
 if(!strcmp(name,"release_step")){controls.release_step=v;return;}
 if(!strcmp(name,"mod_inc")){controls.mod_inc=v;return;}
 fail(line,"unknown set parameter");
}
static int execute(int line,char *cmd){
 char *op=strtok(cmd," \t\r\n");if(!op||op[0]=='#')return 0;
 commands++;
 char *a=strtok(NULL," \t\r\n"),*b=strtok(NULL," \t\r\n");
 if(!strcmp(op,"set")){if(!a||!b)fail(line,"set NAME VALUE");set_param(line,a,b);}
 else if(!strcmp(op,"render_ms")){uint32_t ms=bounded(line,a,60000);render((uint32_t)((uint64_t)ms*SAMPLE_RATE/1000));}
 else if(!strcmp(op,"render_frames")){render(bounded(line,a,4410000));}
 else if(!strcmp(op,"keys")){keys_down=bounded(line,a,0x7ffffff);}
 else if(!strcmp(op,"noteon")){uint32_t note=bounded(line,a,127),vel=bounded(line,b,127);etch_native_midi(&state,0x90,(uint8_t)note,(uint8_t)vel);note_events++;}
 else if(!strcmp(op,"noteoff")){uint32_t note=bounded(line,a,127);etch_native_midi(&state,0x80,(uint8_t)note,0);note_events++;}
 else if(!strcmp(op,"panic")){etch_native_midi_panic(&state);}
 else if(!strcmp(op,"seq_poly")){
  uint16_t step=(uint16_t)bounded(line,a,255),note=(uint8_t)bounded(line,b,127);
  char *velo=strtok(NULL," \t\r\n"),*len=strtok(NULL," \t\r\n");
  if(!etch_seq_put(&pattern,step,note,(uint8_t)bounded(line,velo,127),(uint8_t)bounded(line,len,255)))fail(line,"seq_poly invalid or full");
  controls.pattern=&pattern;
 }
 else if(!strcmp(op,"seq_hold")){
  uint16_t step=(uint16_t)bounded(line,a,255);uint8_t note=(uint8_t)bounded(line,b,127);
  if(etch_seq_hold_toggle(&pattern,step,note)!=1)fail(line,"seq_hold duplicate or full");
  controls.pattern=&pattern;
 }
 else if(!strcmp(op,"seq_bake")){
  uint16_t step=(uint16_t)bounded(line,a,255);uint8_t note=(uint8_t)bounded(line,b,127);
  uint8_t len=(uint8_t)bounded(line,strtok(NULL," \t\r\n"),255);
  if(!etch_seq_bake_chord(&pattern,step,note,&controls.harmony,110,len))fail(line,"chord BAKE failed");
  controls.pattern=&pattern;
 }
 else if(!strcmp(op,"check_hold_span")){
  uint16_t step=(uint16_t)bounded(line,a,255);uint8_t note=(uint8_t)bounded(line,b,127);
  uint32_t expected=bounded(line,strtok(NULL," \t\r\n"),256);
  require(line,etch_seq_hold_span(&pattern,controls.seq_length,step,note)==expected,"authored HOLD span mismatch");
 }
 else if(!strcmp(op,"check_held")){
  uint16_t pitch=(uint16_t)bounded(line,a,127);uint32_t expected=bounded(line,b,8),found=0;
  for(uint8_t i=0;i<ETCH_NATIVE_VOICES;i++)if(state.voice[i].active&&state.voice[i].held&&state.voice[i].note==(uint16_t)(4096u+pitch))found++;
  require(line,found==expected,"held pitch/instance count mismatch");
 }
 else if(!strcmp(op,"seq_lock")){
  uint16_t step=(uint16_t)bounded(line,a,255);uint32_t key=bounded(line,b,5);
  uint16_t value=(uint16_t)bounded(line,strtok(NULL," \t\r\n"),32767);
  pattern.steps[step].lock_mask|=(uint8_t)(1u<<key);pattern.steps[step].locks[key]=value;
  controls.pattern=&pattern;
 }
 else if(!strcmp(op,"midi_start"))etch_native_midi(&state,0xfa,0,0);
 else if(!strcmp(op,"midi_stop"))etch_native_midi(&state,0xfc,0,0);
 else if(!strcmp(op,"midi_clock"))etch_native_midi(&state,0xf8,0,0);
 else if(!strcmp(op,"wave_fx")){
  uint8_t wave=(uint8_t)bounded(line,a,2),key=(uint8_t)bounded(line,b,7);
  uint16_t value=(uint16_t)bounded(line,strtok(NULL," \t\r\n"),32767);
  uint16_t *params=(uint16_t*)&editor.fx[wave];params[key]=value;
  etch_editor_compile(&editor,&compiled_bank);controls.bank=&compiled_bank;
 }
 else if(!strcmp(op,"wave_bake")){
  editor.wave=(uint8_t)bounded(line,a,2);etch_editor_bake_fx(&editor);
  etch_editor_compile(&editor,&compiled_bank);controls.bank=&compiled_bank;
 }
 else if(!strcmp(op,"seq_note")){uint32_t step=bounded(line,a,63),note=bounded(line,b,255);if(note>127&&note!=255)fail(line,"seq_note must be 0..127 or 255 for rest");controls.seq_notes[step]=(uint8_t)note;}
 else if(!strcmp(op,"bank")){
  if(!a)fail(line,"bank on/off");
  if(!strcmp(a,"on")){etch_editor_compile(&editor,&compiled_bank);controls.bank=&compiled_bank;}
  else if(!strcmp(a,"off"))controls.bank=NULL;
  else fail(line,"bank on/off");
 }
 else if(!strcmp(op,"wave_tool")){
  uint32_t wave=bounded(line,a,2),tool=bounded(line,b,11);
  char *p=strtok(NULL," \t\r\n");
  int32_t arg=p?(int32_t)bounded(line,p,127):0;
  editor.wave=(uint8_t)wave;etch_editor_tool(&editor,(uint8_t)tool,arg);
  etch_editor_compile(&editor,&compiled_bank);controls.bank=&compiled_bank;
 }
 else if(!strcmp(op,"check_audio")){
  if(!a)fail(line,"check_audio nonzero/silent");
  if(!strcmp(a,"nonzero"))require(line,abs_since_check>10000,"expected audible output since last check");
  else if(!strcmp(a,"silent"))require(line,abs_since_check==0,"expected silent output since last check");
  else fail(line,"check_audio nonzero/silent");
  abs_since_check=0;frames_since_check=0;
 }
 else if(!strcmp(op,"check_voices")){
  uint32_t expected=bounded(line,a,8),actual=0;
  for(int i=0;i<ETCH_NATIVE_VOICES;i++)actual+=state.voice[i].active!=0;
  require(line,expected==actual,"active voice count mismatch");
 }
 else if(!strcmp(op,"check_perf_events"))require(line,perf_events>=number(line,a),"performance notes did not advance");
 else fail(line,"unknown command");
 return 1;
}
int main(int argc,char **argv){
 if(argc!=4){fprintf(stderr,"usage: etch_host SCENARIO.etch OUTPUT.wav RESULT.json\n");return 1;}
 FILE *script=fopen(argv[1],"r");if(!script){perror(argv[1]);return 1;}
 wav=fopen(argv[2],"wb");if(!wav){perror(argv[2]);fclose(script);return 1;}
 wav_begin(wav);reset();
 char linebuf[512];int n=0;
 while(fgets(linebuf,sizeof(linebuf),script)){n++;execute(n,linebuf);}
 if(ferror(script)){perror("scenario read");return 3;}
 fclose(script);wav_end(wav);
 FILE *json=fopen(argv[3],"w");if(!json){perror(argv[3]);return 1;}
 fprintf(json,"{\n  \"sample_rate\": %u,\n  \"frames\": %" PRIu64 ",\n  \"duration_seconds\": %.3f,\n  \"peak_absolute\": %u,\n  \"mean_absolute\": %.3f,\n  \"clipped_samples\": %" PRIu64 ",\n  \"max_active_voices\": %u,\n  \"performance_events\": %u,\n  \"note_events\": %u,\n  \"assertions_passed\": %u,\n  \"script_commands\": %u\n}\n",SAMPLE_RATE,frames_written,(double)frames_written/SAMPLE_RATE,peak,frames_written?(double)abs_total/frames_written:0.0,clips,max_voices,perf_events,note_events,assertion_count,commands);
 if(fclose(json))return 3;
 printf("ETCH host PASS: %s -> %s (%" PRIu64 " frames, %u checks, peak %u)\n",argv[1],argv[2],frames_written,assertion_count,peak);
 return 0;
}
