import {clamp} from './dsp.mjs';
import {makeHarmony,validateHarmony,effectiveStepNotes} from './harmony.mjs';
import {FixedQueue} from './fixed-queue.mjs';

export const ARP_MODES=['OFF','UP','DOWN','UPDOWN','ORDER','RANDOM'];
export const RATES=['1/4','1/8','1/16','1/32'];
export const LOCK_KEYS=['morph','depth','cutoff','chorus','delay','reverb'];
export const MAX_STEP_NOTES=8;
export const SEQ_LENGTHS=[8,16,32,64,128,256];
export const MAX_SEQ_STEPS=256;
const RATE_BEATS={'1/4':1,'1/8':.5,'1/16':.25,'1/32':.125};

function emptyStep(){return {notes:[],locks:{}};}
export function makePerformance(){return {arp:{mode:'OFF',rate:'1/16',octaves:1,gate:.70},harmony:makeHarmony(),seq:{length:16,rate:'1/16',swing:0,gate:.75,steps:Array.from({length:MAX_SEQ_STEPS},emptyStep)}};}
export function rateBeats(rate){return RATE_BEATS[rate]??.25;}
function finite(v,a,b,name){if(typeof v!=='number'||!Number.isFinite(v)||v<a||v>b)throw Error(`Invalid ${name}`);return v;}
function normalizeNoteEntry(entry,index,stepIndex){
 if(!entry||typeof entry!=='object'||Array.isArray(entry))throw Error(`Invalid sequence note ${stepIndex+1}.${index+1}`);
 const note=entry.note;if(!Number.isInteger(note)||note<0||note>127)throw Error(`Invalid sequence note ${stepIndex+1}.${index+1}`);
 const velocity=finite(entry.velocity??.85,0,1,`sequence velocity ${stepIndex+1}.${index+1}`);
 const length=entry.length??1;if(!Number.isInteger(length)||length<1||length>MAX_SEQ_STEPS)throw Error(`Invalid sequence length ${stepIndex+1}.${index+1}`);
 return {note,velocity,length};
}
function normalizeStep(step,index){
 if(!step||typeof step!=='object'||Array.isArray(step))throw Error(`Invalid sequence step ${index+1}`);
 let notes=[];
 if(Array.isArray(step.notes)){
  if(step.notes.length>MAX_STEP_NOTES)throw Error(`Sequence step ${index+1} exceeds ${MAX_STEP_NOTES} notes`);
  notes=step.notes.map((n,i)=>normalizeNoteEntry(n,i,index));
 }else if('note' in step){
  // V0.8 / bank-format-6 compatibility: one note + one velocity.
  const note=step.note??-1;
  if(!Number.isInteger(note)||note< -1||note>127)throw Error(`Invalid sequence note ${index+1}`);
  if(note>=0)notes=[{note,velocity:finite(step.velocity??.85,0,1,`sequence velocity ${index+1}`),length:1}];
 }
 const seen=new Set();for(const n of notes){if(seen.has(n.note))throw Error(`Duplicate sequence note ${n.note} on step ${index+1}`);seen.add(n.note);}
 notes.sort((a,b)=>a.note-b.note);
 const locks={};if(step.locks!==undefined){if(!step.locks||typeof step.locks!=='object'||Array.isArray(step.locks))throw Error(`Invalid sequence locks ${index+1}`);for(const [key,val] of Object.entries(step.locks)){if(!LOCK_KEYS.includes(key))throw Error(`Invalid sequence lock ${key}`);if(typeof val!=='number'||!Number.isFinite(val))throw Error(`Invalid sequence lock ${key}`);const [lo,hi]=key==='cutoff'?[20,18000]:[0,1];if(val<lo||val>hi)throw Error(`Invalid sequence lock ${key}`);locks[key]=val;}}
 return {notes,locks};
}
export function validatePerformance(value){
 const d=makePerformance(),x=value&&typeof value==='object'&&!Array.isArray(value)?value:{};
 const a=x.arp&&typeof x.arp==='object'?x.arp:{};if('mode'in a&&!ARP_MODES.includes(a.mode))throw Error('Invalid arp mode');if('rate'in a&&!RATES.includes(a.rate))throw Error('Invalid arp rate');
 d.arp.mode=a.mode??d.arp.mode;d.arp.rate=a.rate??d.arp.rate;if('octaves'in a){if(!Number.isInteger(a.octaves)||a.octaves<1||a.octaves>4)throw Error('Invalid arp octaves');d.arp.octaves=a.octaves;}if('gate'in a)d.arp.gate=finite(a.gate,.05,1,'arp gate');
 d.harmony=validateHarmony(x.harmony);
 const q=x.seq&&typeof x.seq==='object'?x.seq:{};if('length'in q&&!SEQ_LENGTHS.includes(q.length))throw Error('Invalid sequence length');if('rate'in q&&!RATES.includes(q.rate))throw Error('Invalid sequence rate');
 d.seq.length=q.length??d.seq.length;d.seq.rate=q.rate??d.seq.rate;if('swing'in q)d.seq.swing=finite(q.swing,0,.75,'sequence swing');if('gate'in q)d.seq.gate=finite(q.gate,.05,1,'sequence gate');
 if(q.steps!==undefined){if(!Array.isArray(q.steps))throw Error('Sequence steps must be an array');if(q.steps.length===64){const migrated=[...q.steps,...Array.from({length:MAX_SEQ_STEPS-64},emptyStep)];d.seq.steps=migrated.map(normalizeStep);}else if(q.steps.length===MAX_SEQ_STEPS)d.seq.steps=q.steps.map(normalizeStep);else throw Error(`Sequence requires 64 legacy or ${MAX_SEQ_STEPS} steps`);}
 return d;
}

function arpNotes(held,mode,octaves){
 const order=[...held.keys()],sorted=[...order].sort((a,b)=>a-b);let base=mode==='ORDER'?order:sorted;
 if(mode==='DOWN')base=[...sorted].reverse();
 let notes=[];for(let o=0;o<octaves;o++)for(const n of base){const note=n+12*o;if(note<=127)notes.push(note);}
 if(mode==='UPDOWN'&&notes.length>1)notes=[...notes,...notes.slice(1,-1).reverse()];
 return notes;
}

export class PerformanceScheduler{
 constructor(sampleRate=44100,offCapacity=4096){if(!Number.isFinite(sampleRate)||sampleRate<8000)throw Error('Invalid scheduler sample rate');this.sr=sampleRate;this.performance=makePerformance();this.baseParams={};this.position=0;this.seqPlaying=false;this.seqStep=0;this.seqNext=0;this.arpNext=0;this.arpIndex=0;this.held=new Map();this.heldOrder=[];this.offs=new FixedQueue(offCapacity);this.queueOverflow=false;this.seqLed=new Map();this.seqInstance=1;this.random=0x12345678;}
 setPerformance(value){this.performance=validatePerformance(value);if(this.seqStep>=this.performance.seq.length)this.seqStep=0;}
 setBaseParams(params){this.baseParams={...params};}
 heldOn(note,velocity=.85){if(!Number.isInteger(note)||note<0||note>127)return;this.held.delete(note);this.held.set(note,clamp(velocity,0,1));this.heldOrder=this.heldOrder.filter(n=>n!==note);this.heldOrder.push(note);if(this.held.size===1){this.arpNext=this.position;this.arpIndex=0;}}
 heldOff(note){this.held.delete(note);this.heldOrder=this.heldOrder.filter(n=>n!==note);if(!this.held.size)this.arpIndex=0;}
 start(reset=true){this.seqPlaying=true;if(reset){this.seqStep=0;this.seqInstance=1;}this.seqNext=this.position;this.offs.clear();this.queueOverflow=false;this.seqLed.clear();}
 stop(synth){this.seqPlaying=false;this.offs.clear();this.queueOverflow=false;this.seqLed.clear();if(synth){synth.panic();if(Object.keys(this.baseParams).length)synth.setParams(this.baseParams);}}
 tempoBpm(synth){return synth.externalBpm??synth.p.bpm;}
 stepSamples(rate,bpm){return this.sr*60/Math.max(1,bpm)*rateBeats(rate);}
 stepInterval(stepIndex,rate,bpm,swing){const base=this.stepSamples(rate,bpm);return Math.max(1,base*(1+(stepIndex%2===0?1:-1)*(swing/3)));}
 noteSpanSamples(startStep,length,q,bpm){const span=Math.max(1,Math.min(q.length,length??1));let total=0;for(let k=0;k<span;k++){const step=(startStep+k)%q.length,interval=this.stepInterval(step,q.rate,bpm,q.swing);if(k===span-1){const base=this.stepSamples(q.rate,bpm),tail=Math.max(1,Math.min(base*q.gate,Math.max(1,interval-1)));total+=tail;}else total+=interval;}return Math.max(1,total);}
 nextRandom(max){this.random=(1664525*this.random+1013904223)>>>0;return max?this.random%max:0;}
 collect(frames,bpm){const end=this.position+frames,events=[];
  if(this.seqPlaying){const q=this.performance.seq;let guard=0;while(this.seqNext<end&&guard++<256){const stepIndex=this.seqStep,step=q.steps[stepIndex],notes=effectiveStepNotes(step,this.performance.harmony),at=this.seqNext,interval=this.stepInterval(stepIndex,q.rate,bpm,q.swing),triggerNotes=[];for(const n of notes){const offTime=at+this.noteSpanSamples(stepIndex,n.length,q,bpm),instance=this.seqInstance++;if(this.seqInstance>0x7fffffff)this.seqInstance=1;const trigger={...n,instance};triggerNotes.push(trigger);if(!this.offs.push({time:offTime,note:n.note,source:'seq',instance}))this.queueOverflow=true;}events.push({offset:Math.max(0,Math.floor(at-this.position)),type:'seq',step:stepIndex,data:{notes:triggerNotes,locks:step.locks}});this.seqStep=(this.seqStep+1)%q.length;this.seqNext+=interval;}}
  else if(this.performance.arp.mode!=='OFF'&&this.held.size){const a=this.performance.arp;let guard=0;while(this.arpNext<end&&guard++<256){let list=arpNotes(new Map(this.heldOrder.filter(n=>this.held.has(n)).map(n=>[n,this.held.get(n)])),a.mode,a.octaves);if(list.length){let index=a.mode==='RANDOM'?this.nextRandom(list.length):this.arpIndex%list.length,note=list[index],base=note%12;let velocity=.85;for(const [n,v] of this.held){if(n%12===base){velocity=v;break;}}events.push({offset:Math.max(0,Math.floor(this.arpNext-this.position)),type:'on',note,velocity});const step=this.stepSamples(a.rate,bpm);if(!this.offs.push({time:this.arpNext+Math.max(1,step*a.gate),note,source:'arp'}))this.queueOverflow=true;this.arpIndex++;this.arpNext+=Math.max(1,step);}else break;}}
  this.offs.drainBefore(end,e=>events.push({offset:Math.max(0,Math.floor(e.time-this.position)),type:'off',note:e.note,source:e.source,instance:e.instance??0}));
  events.sort((a,b)=>a.offset-b.offset||((a.type==='off'?0:a.type==='seq'?1:2)-(b.type==='off'?0:b.type==='seq'?1:2)));return events;}
 process(synth,out){const bpm=this.tempoBpm(synth),events=this.collect(out.length,bpm);if(this.queueOverflow){this.seqPlaying=false;this.offs.clear();this.seqLed.clear();synth.panic();if(Object.keys(this.baseParams).length)synth.setParams(this.baseParams);this.position+=out.length;out.fill(0);this.queueOverflow=false;return {seqPlaying:false,seqStep:this.seqStep,triggeredStep:null,seqLedChanged:true,seqLedNotes:[],queueOverflow:true};}let cursor=0,triggeredStep=null,seqLedChanged=false;for(const e of events){const at=Math.min(out.length,Math.max(cursor,e.offset));if(at>cursor)synth.process(out.subarray(cursor,at));cursor=at;if(e.type==='off'){synth.noteOff(e.note,e.instance??0);if(e.source==='seq'){const count=this.seqLed.get(e.note)||0;if(count<=1){if(this.seqLed.delete(e.note))seqLedChanged=true;}else this.seqLed.set(e.note,count-1);}}else if(e.type==='on')synth.noteOn(e.note,e.velocity);else if(e.type==='seq'){triggeredStep=e.step;synth.applyPerformanceLocks(this.baseParams,e.data.locks);for(const n of e.data.notes){synth.noteOn(n.note,n.velocity,n.instance??0);const count=this.seqLed.get(n.note)||0;this.seqLed.set(n.note,count+1);if(count===0)seqLedChanged=true;}}}
  if(cursor<out.length)synth.process(out.subarray(cursor));this.position+=out.length;return {seqPlaying:this.seqPlaying,seqStep:(this.seqStep+this.performance.seq.length-1)%this.performance.seq.length,triggeredStep,seqLedChanged,seqLedNotes:[...this.seqLed.keys()].sort((a,b)=>a-b),queueOverflow:false};}
}
