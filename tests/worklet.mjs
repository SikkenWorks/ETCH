// Exercise actual worklet message/render integration without a browser audio device.
import assert from 'node:assert/strict';
import {compileWave,primitive} from '../sim/dsp.mjs';
import {makePerformance} from '../sim/performance.mjs';
let Processor;const workletMessages=[];
globalThis.sampleRate=44100;
globalThis.AudioWorkletProcessor=class{constructor(){this.port={postMessage:m=>workletMessages.push(m)};}};
globalThis.registerProcessor=(name,type)=>{assert.equal(name,'etch');Processor=type;};
await import('../sim/worklet.mjs');
const p=new Processor(),wave=compileWave(primitive('SINE'));
p.port.onmessage({data:{type:'bank',bank:[wave,wave,wave]}});
p.port.onmessage({data:{type:'params',params:{depth:.4,mode:'TEMPO',bars:.25}}});
p.port.onmessage({data:{type:'clock',bpm:128}});
p.port.onmessage({data:{type:'on',note:60,velocity:1}});
let peak=0;for(let b=0;b<80;b++){const out=new Float32Array(128);assert.equal(p.process([],[[out]]),true);for(const x of out){assert(Number.isFinite(x));peak=Math.max(peak,Math.abs(x));}}
assert(peak>.001);assert(workletMessages.some(m=>m.type==='telemetry'&&m.voices===1&&m.peak>0));
const perf=makePerformance();perf.seq.steps[0]={notes:[{note:60,velocity:.8},{note:64,velocity:.7}],locks:{}};p.port.onmessage({data:{type:'performance',performance:perf}});p.port.onmessage({data:{type:'transport',action:'start',reset:true}});p.process([],[[new Float32Array(128)]]);assert(workletMessages.some(m=>m.type==='seq-step'&&m.seqStep===0));assert(workletMessages.some(m=>m.type==='seq-led'&&m.notes.includes(60)&&m.notes.includes(64)),'AudioWorklet must forward scheduler key-LED note state');
p.port.onmessage({data:{type:'clock',bpm:null}});p.port.onmessage({data:{type:'panic'}});assert(workletMessages.some(m=>m.type==='seq-led'&&m.notes.length===0));
for(let b=0;b<3;b++){const out=new Float32Array(128);p.process([],[[out]]);if(b>0)assert(out.every(x=>x===0));}
console.log('AudioWorklet message, external clock, telemetry, sequence key-LED and block integration passed (host harness)');
