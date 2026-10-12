import assert from 'node:assert/strict';
import {drawScreen,WAVEFORM_VIEW,scopeDisplaySamples} from '../sim/screen.mjs';
import {compileWave,primitive,defaults} from '../sim/dsp.mjs';

assert(WAVEFORM_VIEW.PLAY.height > WAVEFORM_VIEW.OTHER.height);
assert(WAVEFORM_VIEW.MORPH.height > WAVEFORM_VIEW.PLAY.height);
assert(WAVEFORM_VIEW.PLAY.top+WAVEFORM_VIEW.PLAY.height < 180);
assert(WAVEFORM_VIEW.MORPH.top+WAVEFORM_VIEW.MORPH.height < 186);

const input=Array.from({length:128},(_,i)=>.018*Math.sin(2*Math.PI*i/128));
const before=[...input],enlarged=scopeDisplaySamples(input,{live:true});
assert(Math.max(...enlarged.map(Math.abs))>.75,'Live trace should occupy the graph');
assert.deepEqual(input,before,'Autoscale must never alter DSP samples');
assert.deepEqual(scopeDisplaySamples(Array(128).fill(.0001),{live:true}),Array(128).fill(.0001),'Near-silence must not be amplified');
assert(scopeDisplaySamples([NaN,Infinity,-Infinity],{live:true}).every(x=>Number.isFinite(x)));

const captures={};
function fakeCanvas(){
 const rectangles=[],texts=[];
 const context={fillStyle:'',strokeStyle:'',lineWidth:1,font:'',textAlign:'left',
  fillRect(x,y,w,h){assert(x>=0&&y>=0&&x+w<=240&&y+h<=240);},
  strokeRect(){},beginPath(){},moveTo(){},lineTo(){},stroke(){},
  save(){},restore(){},clip(){},rect(x,y,w,h){rectangles.push({x,y,w,h});},
  fillText(s,x,y){texts.push({s:String(s),x,y});}
 };
 return {context,rectangles,texts};
}
const tables=[compileWave(primitive('SINE')),compileWave(primitive('TRIANGLE')),compileWave(primitive('SINE'))];
for(const page of ['PLAY','MORPH','FILTER']){
 const target=fakeCanvas();
 drawScreen(target.context,{page,points:primitive('SINE'),bank:tables,slots:[0,1,2],
  params:{...defaults},waveNames:['SINE','TRIANGLE','SINE'],battery:{present:false},
  audioData:page==='PLAY'?Float32Array.from(input):null});
 const plot=target.rectangles.find(r=>r.w===212);
 assert(plot,`Waveform must be clipped to graph bounds on ${page}`);
 assert.equal(plot.y,WAVEFORM_VIEW[page]?.top??WAVEFORM_VIEW.OTHER.top);
 assert.equal(plot.h,WAVEFORM_VIEW[page]?.height??WAVEFORM_VIEW.OTHER.height);
 assert(target.texts.every(t=>t.y<=240&&t.y>=0));
 captures[page]={top:plot.y,height:plot.h};
}
console.log('PASS V0.51 PLAY/MORPH enlarged plot and display-only live auto-scaling',captures);