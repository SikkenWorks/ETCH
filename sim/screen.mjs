import {previewSample,readWave} from './dsp.mjs';
import {NOTE_NAMES,scalePitchClasses,circleOfFifthsChords,effectiveStepNotes,midiNoteName} from './harmony.mjs';

export const SCREEN_MIN_FONT=12;
const C={bg:'#07100d',ink:'#f4f6e8',muted:'#b7c3b5',grid:'#294033',accent:'#d6ff82',cyan:'#65e8ef',amber:'#ffc968',magenta:'#ff86df',red:'#ff755f',panel:'#0c1712'};

// The oscilloscope is the focus of PLAY/MORPH, not an unscaled thumbnail.
// These are display-only coordinates; DSP gain and stored samples are unchanged.
export const WAVEFORM_VIEW={
 PLAY:{top:38,height:137,left:14,width:212},
 MORPH:{top:38,height:145,left:14,width:212},
 OTHER:{top:42,height:112,left:14,width:212}
};
export function scopeDisplaySamples(samples,{live=false}={}){
 const source=Array.from(samples||[],v=>Number.isFinite(v)?v:0);
 if(!source.length)return source;
 const peak=source.reduce((max,v)=>Math.max(max,Math.abs(v)),0);
 // Do not turn background noise/silence into a giant waveform. The gain is
 // deliberately bounded and applies ONLY to the visual, never to audio.
 const gain=live&&peak>=.005?Math.min(64,.88/peak):1;
 return source.map(v=>Math.max(-1,Math.min(1,v*gain)));
}

export function drawScreen(g,{page,subpage={},dirty,cursor,points,bank,slots,params,performance=null,currentSeqStep=0,seqPlaying=false,seqRecording=false,harmonyScope='STEP',booting=false,audioData=null,waveSave=null,waveNames=[],editFocus='A',editWaveName='',waveFx=null,fxPreview=null,battery={percent:100,charging:false,present:true},saveManager=null}){
 const pct=v=>`${Math.round(v*100)}%`;
 const clip=(s,size=SCREEN_MIN_FONT,width=212)=>{s=String(s);const max=Math.max(2,Math.floor(width/(Math.max(SCREEN_MIN_FONT,size)*.62)));return s.length<=max?s:s.slice(0,Math.max(1,max-2))+'..';};
 function text(s,x,y,color=C.ink,size=10,align='left',weight='600',width=212){size=Math.max(SCREEN_MIN_FONT,size);g.fillStyle=color;g.font=`${weight} ${size}px monospace`;g.textAlign=align;g.fillText(clip(s,size,width),x,y);g.textAlign='left';}
 function line(y,color=C.grid){g.strokeStyle=color;g.lineWidth=1;g.beginPath();g.moveTo(12,y+.5);g.lineTo(228,y+.5);g.stroke();}
 function batteryIndicator(){if(!battery?.present)return;const pct=Math.max(0,Math.min(100,Math.round(battery.percent??0))),x=204,y=10,w=21;g.strokeStyle=battery.charging?C.accent:C.muted;g.lineWidth=1;g.strokeRect(x+.5,y+.5,w,10);g.fillStyle=battery.charging?C.accent:C.muted;g.fillRect(x+w+1,y+3,2,5);const inner=Math.max(0,Math.round((w-4)*pct/100));if(inner){g.fillStyle=pct<=15?C.red:battery.charging?C.accent:C.ink;g.fillRect(x+2,y+2,inner,7);}text(`${pct}%`,199,21,pct<=15?C.red:battery.charging?C.accent:C.muted,11,'right','700',34);if(battery.charging){g.strokeStyle=C.bg;g.lineWidth=1.4;g.beginPath();g.moveTo(x+11,y+2);g.lineTo(x+8,y+6);g.lineTo(x+12,y+6);g.lineTo(x+9,y+10);g.stroke();}}
 function header(title){text('ETCH',10,22,C.accent,16,'left','700',44);text(dirty?`${title} *`:title,60,22,C.ink,13,'left','700',108);batteryIndicator();line(34);}
 function graphGrid(y0=42,h=112){g.strokeStyle=C.grid;g.lineWidth=.5;for(let i=0;i<=8;i++){const x=14+212*i/8;g.beginPath();g.moveTo(x,y0);g.lineTo(x,y0+h);g.stroke();}for(let i=0;i<=4;i++){const y=y0+h*i/4;g.beginPath();g.moveTo(14,y);g.lineTo(226,y);g.stroke();}}
 function trace(values,color,width=1.25,y0=42,h=112,x0=14,w=212,amp=.43){if(!values?.length)return;g.save();g.beginPath();g.rect(x0,y0,w,h);g.clip();g.strokeStyle=color;g.lineWidth=width;g.beginPath();const mid=y0+h/2,scale=h*amp;for(let i=0;i<values.length;i++){const x=x0+w*i/Math.max(1,values.length-1),y=mid-scale*values[i];i?g.lineTo(x,y):g.moveTo(x,y);}g.stroke();g.restore();}
 function miniTrace(values,color,x0,y0,w=82,h=26){if(!values?.length)return;g.save();g.beginPath();g.rect(x0,y0,w,h);g.clip();g.strokeStyle=color;g.lineWidth=1.1;g.beginPath();for(let i=0;i<values.length;i++){const x=x0+w*i/Math.max(1,values.length-1),y=y0+h/2-h*.40*values[i];i?g.lineTo(x,y):g.moveTo(x,y);}g.stroke();g.restore();}
 function wrapTokens(tokens,maxChars=30){const lines=[];let row='';for(const token of tokens){const candidate=row?`${row}  ${token}`:token;if(candidate.length>maxChars&&row){lines.push(row);row=token;}else row=candidate;}if(row)lines.push(row);return lines.slice(0,2);}
 function waveLabel(index){const name=waveNames[slots[index]]||`WAVE ${String(slots[index]+1).padStart(2,'0')}`;return `${['A','B','C'][index]} ${String(slots[index]+1).padStart(2,'0')} ${name}`;}

 g.imageSmoothingEnabled=false;g.fillStyle=C.bg;g.fillRect(0,0,240,240);
 if(booting){text('SikkenWorks',120,108,C.ink,19,'center','700',220);text('Born out of passion',120,132,C.muted,10,'center','600',210);return;}

 if(saveManager?.active){
  header('MEMORY');
  const actions=['SAVE','SAVE AS','LOAD','NEW'],m=saveManager.name||{chars:Array(12).fill(' '),cursor:0,length:1};
  const editable=saveManager.action==='SAVE AS',stored=saveManager.action==='LOAD',name=m.chars.slice(0,m.length).join('').trim()||saveManager.type;
  text(saveManager.type,14,54,C.accent,14,'left','700',120);
  const tabY=63,tabW=52.5;for(let i=0;i<actions.length;i++){const x=14+i*tabW,on=saveManager.action===actions[i];g.fillStyle=on?'#20361f':C.panel;g.fillRect(x,tabY,tabW-2,24);g.strokeStyle=on?C.accent:C.grid;g.lineWidth=on?1.5:1;g.strokeRect(x+.5,tabY+.5,tabW-3,23);text(actions[i],x+(tabW-2)/2,tabY+16,on?C.accent:C.muted,11,'center','700',tabW-2);}
  const slotNeeded=['SAVE AS','LOAD'].includes(saveManager.action),slotText=slotNeeded?String(saveManager.slot+1).padStart(2,'0'):'—';
  text(`SLOT ${slotText}`,14,106,C.ink,12,'left','700',90);
  if(editable){
   text('NAME',14,125,C.muted,11,'left','700');const boxW=17,boxGap=1,x0=13,y0=132;for(let i=0;i<12;i++){const x=x0+i*(boxW+boxGap),active=i===m.cursor;g.fillStyle=active?'#20361f':C.panel;g.fillRect(x,y0,boxW,28);g.strokeStyle=active?C.accent:C.grid;g.lineWidth=active?1.5:1;g.strokeRect(x+.5,y0+.5,boxW-1,27);text(m.chars[i]===' '?'·':m.chars[i],x+boxW/2,y0+19,active?C.accent:C.ink,12,'center','700',boxW);}
   text(name,14,178,C.ink,12,'left','700');text('K1 OBJ  K2 ACTION',14,200,C.muted,11,'left','700');text('K3 SLOT  K4 LENGTH',14,218,C.muted,11,'left','700');text('SELECT POS  ALG CHAR',14,236,C.cyan,11,'left','700');
  }else{
   const display=saveManager.action==='NEW'?`NEW ${saveManager.type}`:name;
   text(stored?'SAVED':'CURRENT',14,131,C.muted,11,'left','700');text(display,14,153,saveManager.action==='NEW'?C.accent:C.ink,14,'left','700');
   text(saveManager.action==='SAVE'?'OVERWRITE CURRENT SLOT':saveManager.action==='LOAD'?'LOAD SELECTED SLOT':'RESET CURRENT OBJECT',14,178,saveManager.action==='NEW'?C.amber:C.cyan,11,'left','700');
   text('K1 OBJ  K2 ACTION',14,202,C.muted,11,'left','700');text(slotNeeded?'K3 SLOT':'K3 —',14,220,C.muted,11,'left','700');text('SAVE = EXECUTE',226,238,C.accent,11,'right','700',120);
  }
  return;
 }

 if(waveSave?.active){
  header('SAVE WAVE');
  text(`SLOT ${String(waveSave.slot+1).padStart(2,'0')}  LEN ${waveSave.length}`,14,57,C.cyan,12,'left','700');text('NAME',14,80,C.muted,11);
  const boxW=17,boxGap=1,x0=13,y0=88;for(let i=0;i<12;i++){const x=x0+i*(boxW+boxGap),active=i===waveSave.cursor;g.fillStyle=active?'#20361f':C.panel;g.fillRect(x,y0,boxW,30);g.strokeStyle=active?C.accent:C.grid;g.lineWidth=active?1.5:1;g.strokeRect(x+.5,y0+.5,boxW-1,29);text(waveSave.chars[i]===' '?'·':waveSave.chars[i],x+boxW/2,y0+20,active?C.accent:C.ink,12,'center','700',boxW);}
  text(`POS ${String(waveSave.cursor+1).padStart(2,'0')}  CHAR ${waveSave.chars[waveSave.cursor]===' '?'<SPACE>':waveSave.chars[waveSave.cursor]}`,14,142,C.ink,12,'left','700');line(154);text('K1 SLOT  K2 POS',14,177,C.muted,11);text('K3 CHAR  K4 LEN',14,197,C.muted,11);text('SELECT POS',14,220,C.cyan,11);text('ALG CHAR  SAVE OK',226,238,C.accent,11,'right','700',150);return;
 }

 const title=page==='PLAY'?'SOUND':page==='ENV'?(subpage.ENV?'ENV FILTER':'ENV AMP'):page==='MOD WAVE'?(subpage['MOD WAVE']?'MOD RATE':'MOD WAVE'):page==='MORPH'&&subpage.MORPH?'MORPH PHASE':page==='SEQ'?(subpage.SEQ===1?'SEQ STEP':subpage.SEQ===2?'SEQ HARMONY':subpage.SEQ===3?'WAVE SEQ':'SEQUENCER'):page;
 header(title);

 if(page==='SEQ'&&performance){
  const q=performance.seq,h=performance.harmony,step=q.steps[currentSeqStep]||{notes:[],locks:{}};
  const incomingHold=[];for(let start=0;start<currentSeqStep;start++)for(const n of effectiveStepNotes(q.steps[start],h))if(start+(n.length??1)>currentSeqStep)incomingHold.push(n);
  if(subpage.SEQ===3){
   const stepNum=String(currentSeqStep+1).padStart(2,'0'),locked=step.locks?.morph!==undefined;
   const value=locked?step.locks.morph:0,where=!locked?'NO LOCK':value<=0?'WAVE A':value>=1?'WAVE B':'A/B MORPH';
   text(`STEP ${stepNum} / ${q.length}`,14,54,C.accent,12,'left','700');
   text('WAVE SOURCE',14,86,C.muted,10);text(where,14,109,C.cyan,14,'left','700');
   text(`MORPH ${Math.round(value*100)}%`,14,141,C.amber,13);
   g.fillStyle=C.panel;g.fillRect(14,153,212,8);g.fillStyle=C.cyan;g.fillRect(14,153,212*value,8);
   text(locked?'LOCK ON':'LOCK OFF',14,187,locked?C.accent:C.muted,12);
   text('A = 0%     B = 100%',14,208,C.muted,9);
   text('K1 STEP K2 WAVE K3 MIX K4 LOCK',14,232,C.accent,9);return;
  }
  if(subpage.SEQ===2){
   const pcs=scalePitchClasses(h),fifths=circleOfFifthsChords(h),scaleNames=pcs.map(pc=>NOTE_NAMES[pc]),chordLines=wrapTokens(fifths.map(x=>x.name),29);
   text(`${NOTE_NAMES[h.root]} ${h.mode}`,14,55,C.accent,13,'left','700');
   text(`KEYLOCK ${h.keylock}   CHORD ${h.chord}`,14,76,h.keylock==='LOCK'?C.cyan:C.ink,10,'left','700');
   text('SCALE NOTES',14,101,C.muted,9);text(scaleNames.join('  '),14,119,C.ink,10,'left','700');
   text('IN-KEY CHORDS · FIFTHS ORDER',14,143,C.muted,9);text(chordLines[0]||'NONE',14,161,C.amber,10,'left','700');if(chordLines[1])text(chordLines[1],14,179,C.amber,10,'left','700');
   text(`BAKE ${harmonyScope}   AUTO ${h.chord}`,14,204,C.ink,10,'left','700');text('SEQ = GRID',14,229,C.muted,9);text('SEL = BAKE',226,229,C.accent,9,'right','700',90);return;
  }
  if(subpage.SEQ===1){
   const effective=effectiveStepNotes(step,h),labels=step.notes.map(n=>`${midiNoteName(n.note)}${(n.length??1)>1?'×'+n.length:''}`);
   text(`STEP ${String(currentSeqStep+1).padStart(q.length>=100?3:2,'0')} / ${q.length}`,14,54,C.accent,12,'left','700');text(`TRIGGER ${step.notes.length}   HOLD ${incomingHold.length}`,14,75,step.notes.length?C.cyan:incomingHold.length?C.amber:C.muted,10,'left','700');
   for(let i=0;i<labels.length;i++)text(labels[i],14+(i%4)*52,101+Math.floor(i/4)*22,C.cyan,10,'left','700',46);
   if(incomingHold.length){const held=[...new Set(incomingHold.map(n=>midiNoteName(n.note)))];text(`HOLD: ${held.join(' ')}`,14,151,C.amber,9,'left','700');}
   else if(h.chord!=='OFF'&&step.notes.length===1)text(`AUTO ${h.chord}: ${effective.map(n=>midiNoteName(n.note)).join(' ')}`,14,151,C.amber,9,'left','700');
   const locks=Object.keys(step.locks||{}).map(k=>k==='depth'?'ETCH':k.toUpperCase());text(`LOCKS ${locks.length?locks.join(' · '):'NONE'}`,14,176,locks.length?C.magenta:C.muted,9,'left','700');
   text(`${q.rate}  SW ${pct(q.swing)}  GATE ${pct(q.gate)}`,14,199,C.ink,9,'left','700');text(`${NOTE_NAMES[h.root]} ${h.mode}`,14,217,C.muted,9,'left','700');text(seqRecording?'REC':seqPlaying?'PLAY':'STOP',14,234,seqRecording?C.magenta:seqPlaying?C.accent:C.muted,10,'left','700');text('SELECT = STEP  ·  SEL = GRID',226,234,C.ink,9,'right','600',155);return;
  }
  const len=q.length,cols=len<=64?8:16,rows=Math.ceil(len/cols),x0=14,y0=42,w=212,hg=132,cw=w/cols,ch=hg/rows;
  const continuationCount=i=>{let count=0;for(let start=0;start<i;start++)for(const n of q.steps[start].notes)if((n.length??1)>i-start)count++;return count;};
  g.strokeStyle=C.grid;g.lineWidth=.5;
  for(let i=0;i<len;i++){const row=Math.floor(i/cols),col=i%cols,x=x0+col*cw,y=y0+row*ch,st=q.steps[i],count=st.notes.length,continuing=continuationCount(i);g.strokeRect(x+.5,y+.5,cw-1,ch-1);if(continuing){g.fillStyle=C.amber;g.fillRect(x+2,y+3,Math.max(2,cw-4),2);}if(count){g.fillStyle=C.cyan;g.fillRect(x+2,y+ch-5,Math.max(2,cw-4),Math.min(4,1+count));}if(Object.keys(st.locks||{}).length){g.fillStyle=C.magenta;g.fillRect(x+cw-4,y+2,2,3);}if(i===currentSeqStep){g.strokeStyle=seqRecording?C.magenta:C.accent;g.lineWidth=1.5;g.strokeRect(x+1,y+1,cw-2,ch-2);g.strokeStyle=C.grid;g.lineWidth=.5;}}
  const longest=step.notes.reduce((m,n)=>Math.max(m,n.length??1),1),cont=continuationCount(currentSeqStep);
  text(`${len} STEPS  ${q.rate}  SW ${pct(q.swing)}`,14,195,C.ink,9,'left','700');text(`CYAN TRIGGER · AMBER HOLD`,14,214,C.muted,9,'left','700');text(seqRecording?'REC':seqPlaying?'PLAY':'STOP',14,233,seqRecording?C.magenta:seqPlaying?C.accent:C.muted,10,'left','700');const layerText=step.notes.length&&cont?`T${step.notes.length}+H${cont}`:step.notes.length?`T${step.notes.length}${longest>1?' L'+longest:''}`:cont?`H${cont}`:'REST';text(`S${String(currentSeqStep+1).padStart(q.length>=100?3:2,'0')} · ${layerText} · SELECT`,226,233,C.ink,9,'right','700',168);return;
 }

 if(page==='LIBRARY'){
  const colors=[C.cyan,C.amber,C.magenta],letters=['A','B','C'];
  for(let i=0;i<3;i++){const y=45+i*55,focus=letters[i]===editFocus;g.fillStyle=C.panel;g.fillRect(12,y,216,46);g.strokeStyle=focus?C.accent:C.grid;g.lineWidth=focus?1.5:1;g.strokeRect(12.5,y+.5,215,45);text(`${focus?'>':' '} ${waveLabel(i)}`,18,y+19,focus?C.accent:colors[i],10,'left','700',113);text(focus?'EDIT TARGET':'ASSIGNED',18,y+37,focus?C.accent:C.muted,9,'left','600',100);if(bank[i]){const vals=Array.from({length:64},(_,n)=>readWave(bank[i],n/63,261.626,176400));miniTrace(vals,colors[i],139,y+10,80,27);}}
  text('K1=A   K2=B   K3=C   K4=FOCUS',14,218,C.muted,9);text(`PRESS EDIT → WAVE ${editFocus}`,14,235,C.accent,10,'left','700');return;
 }

 const graph=WAVEFORM_VIEW[page]||WAVEFORM_VIEW.OTHER;
 graphGrid(graph.top,graph.height);
 if(page==='DRAW'||page==='WAVE FX'){
  if(fxPreview?.length)trace(fxPreview,C.amber,1.15,graph.top,graph.height);
  trace(points.map(x=>x/32768),C.ink,1.8,graph.top,graph.height);
  if(page==='DRAW'){
   const x=14+212*cursor/127,y=98-48*points[cursor]/32768;g.strokeStyle=C.accent;g.lineWidth=1;g.beginPath();g.moveTo(x,42);g.lineTo(x,154);g.stroke();g.fillStyle=C.accent;g.fillRect(x-2,y-2,5,5);
   text('RAW WHITE · FX AMBER',14,171,C.muted,10,'left','700');text(`DRAW ${editFocus} · ${editWaveName||'UNTITLED'}`,14,191,C.cyan,10,'left','700');text(`POINT ${String(Math.floor(cursor/4)+1).padStart(2,'0')}/32  VALUE ${points[cursor]}`,14,212,C.ink,10,'left','700');text('K1 < > HORIZ   K2 < > VERT',14,236,C.accent,10,'left','700');return;
  }
  const f=waveFx||{};text(`WAVE ${editFocus} · RAW / FX`,14,173,C.cyan,10,'left','700');
  if(subpage['WAVE FX']){text(`REV ${pct(f.reverb||0)}  FOLD ${pct(f.fold||0)}`,14,195,C.ink,10,'left','700');text(`SMOOTH ${pct(f.smooth||0)}  MIX ${pct(f.mix??1)}`,14,217,C.ink,10,'left','700');}
  else {text(`DRIVE ${pct(f.drive||0)}  CRUSH ${pct(f.bitcrush||0)}`,14,195,C.ink,10,'left','700');text(`CHORUS ${pct(f.chorus||0)}  DELAY ${pct(f.delay||0)}`,14,217,C.ink,10,'left','700');}
  text('SELECT PAGE · EDIT DRAW',14,238,C.accent,10,'left','700');return;
 }
 if(!bank.length){text('BUILDING WAVES...',14,108,C.muted,10,'left','700');return;}
 const vals=bank.map((w,j)=>Array.from({length:213},(_,i)=>readWave(w,i/212+(j===0?params.phaseA/360:j===1?params.phaseB/360:0),261.626,176400)*(j===0?1-2*params.invertA:j===1?1-2*params.invertB:1)));
 if(page==='MORPH'){trace(vals[0],C.cyan,1.25,graph.top,graph.height);trace(vals[1],C.amber,1.25,graph.top,graph.height);}if(page==='MOD WAVE')trace(vals[2],C.magenta,1.1,graph.top,graph.height);
 let result;
 const liveAudio=['PLAY','FILTER','ENV','FX','ARP'].includes(page)&&audioData;
 if(liveAudio)result=Array.from({length:213},(_,i)=>audioData[Math.floor(i*audioData.length/213)]);
 else result=Array.from({length:213},(_,i)=>previewSample(bank,params,i/212));
 trace(scopeDisplaySamples(result,{live:liveAudio}),C.ink,page==='PLAY'||page==='MORPH'?2.0:1.7,graph.top,graph.height);

 if(page==='PLAY'){
  text('SOUND FIRST  ·  HOLD HOME: PRESETS',14,188,C.muted,10,'left','700',212);
  const macros=[['MORPH',pct(params.morph)],['ETCH',pct(params.depth)],['TONE',`${Math.round(params.cutoff)}`],['SPACE',pct(params.reverb)]];
  for(let i=0;i<4;i++){
   const x=14+i*54;
   text(macros[i][0],x,209,C.muted,10,'left','700',51);
   text(macros[i][1],x,232,i===0?C.cyan:i===1?C.magenta:i===2?C.amber:C.accent,11,'left','700',51);
  }
 }
 else if(page==='MORPH'){
  text(waveLabel(0),14,198,C.cyan,10,'left','700');
  text(waveLabel(1),14,216,C.amber,10,'left','700');
  text(`MORPH ${pct(params.morph)}   EDIT ${editFocus}`,14,235,C.accent,10,'left','700');
 }
 else if(page==='MOD WAVE'){text(waveLabel(2),14,183,C.magenta,10,'left','700');text(`TARGET ${params.target}   DEPTH ${pct(params.depth)}`,14,205,C.ink,10,'left','700');text(subpage['MOD WAVE']?`RATIO ${params.ratio}   FREE ${params.free.toFixed(2)}Hz`:`MODE ${params.mode}   EDIT → C`,14,230,C.accent,9,'left','700');}
 else if(page==='FILTER'){text(`${params.filter}   CUTOFF ${Math.round(params.cutoff)}Hz`,14,190,C.ink,10,'left','700');text(`RESO ${pct(params.resonance)}   ENV ${(params.filterEnv>=0?'+':'')+pct(params.filterEnv)}`,14,216,C.accent,10,'left','700');}
 else if(page==='ENV'){const prefix=subpage.ENV?'FILTER':'AMP',a=subpage.ENV?params.fAttack:params.attack,d=subpage.ENV?params.fDecay:params.decay,s=subpage.ENV?params.fSustain:params.sustain,r=subpage.ENV?params.fRelease:params.release;text(`${prefix} ENVELOPE`,14,187,C.accent,10,'left','700');text(`A ${a.toFixed(2)}   D ${d.toFixed(2)}`,14,209,C.ink,10,'left','700');text(`S ${pct(s)}   R ${r.toFixed(2)}`,14,231,C.ink,10,'left','700');}
 else if(page==='FX'){text(`DRIVE ${pct(params.drive)}   CHORUS ${pct(params.chorus)}`,14,196,C.ink,9,'left','700');text(`DELAY ${pct(params.delay)}   REVERB ${pct(params.reverb)}`,14,224,C.accent,10,'left','700');}
 else if(page==='SYSTEM'){text(`${params.voice}   GLIDE ${params.glide.toFixed(2)}s`,14,196,C.ink,10,'left','700');text(`BEND ${Math.round(params.bendRange)}st   VELOCITY ${pct(params.velSense)}`,14,224,C.accent,10,'left','700');}
 else if(page==='ARP'&&performance){const a=performance.arp;text(`${a.mode}   RATE ${a.rate}`,14,195,C.ink,10,'left','700');text(`OCT ${a.octaves}   GATE ${pct(a.gate)}`,14,224,C.accent,10,'left','700');}
 else{text(`MORPH ${pct(params.morph)}   ETCH ${pct(params.depth)}`,14,213,C.ink,10,'left','700');text(`SPACE ${pct(params.reverb)}`,14,233,C.accent,10,'left','700');}
}
