export function parseMidiMessage(data){
 if(!data||data.length<1)return null;
 const command=data[0];
 if(command===0xf0&&data[data.length-1]===0xf7)return {type:'sysex',data:Uint8Array.from(data)};
 if(command===0xf8)return {type:'clock'};
 if(command===0xfa)return {type:'start'};
 if(command===0xfb)return {type:'continue'};
 if(command===0xfc)return {type:'stop'};
 const status=command&0xf0,channel=command&0x0f,a=data[1]??0,b=data[2]??0;
 if(status===0x90)return b===0?{type:'off',channel,note:a}:{type:'on',channel,note:a,velocity:b/127};
 if(status===0x80)return {type:'off',channel,note:a};
 if(status===0xd0)return {type:'aftertouch',channel,value:a/127};
 if(status===0xe0){const raw=(b<<7)|a;return {type:'bend',channel,value:Math.max(-1,Math.min(1,(raw-8192)/(raw>=8192?8191:8192)))};}
 if(status===0xb0&&a===64)return {type:'sustain',channel,down:b>=64};
 if(status===0xb0&&(a===120||a===123))return {type:'panic',channel};
 if(status===0xb0)return {type:'cc',channel,controller:a,value:b/127,raw:b};
 return null;
}

export class MidiClockTracker{
 constructor(){this.reset();}
 reset(){this.last=null;this.intervals=[];this.bpm=null;}
 tick(timeMs){
  if(!Number.isFinite(timeMs))return this.bpm;
  if(this.last===null){this.last=timeMs;return this.bpm;}
  const dt=timeMs-this.last;this.last=timeMs;
  if(dt<=0||dt>500){this.intervals=[];this.bpm=null;return this.bpm;}
  // MIDI clock is 24 pulses per quarter note. Keep one beat of recent timing
  // and use the median so a single scheduling spike does not swing the tempo.
  this.intervals.push(dt);if(this.intervals.length>24)this.intervals.shift();
  if(this.intervals.length<6)return this.bpm;
  const sorted=[...this.intervals].sort((a,b)=>a-b),median=sorted[Math.floor(sorted.length/2)];
  const bpm=60000/(median*24);this.bpm=bpm>=30&&bpm<=300?bpm:null;return this.bpm;
 }
}

export function ccToParam(controller,value){
 const x=Math.max(0,Math.min(1,Number.isFinite(value)?value:0));
 if(controller===1)return {key:'morph',value:x,label:'MORPH'};
 if(controller===2)return {key:'depth',value:x,label:'ETCH'};
 if(controller===71)return {key:'resonance',value:x,label:'RESONANCE'};
 if(controller===74)return {key:'cutoff',value:20*Math.pow(900,x),label:'CUTOFF'};
 return null;
}
