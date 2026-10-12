import assert from 'node:assert/strict';
import {Synth,primitive,compileWave,decimatorKernel,validateParams} from '../sim/dsp.mjs';
const wave=compileWave(primitive('SINE')),bank=[wave,wave,wave];
function rms(filter,cutoff){const s=new Synth();s.setBank(bank);s.setParams({filter,cutoff,attack:.001,decay:.001,sustain:1});s.noteOn(69);s.process(new Float32Array(10000));const x=new Float32Array(8192);s.process(x);return Math.sqrt(x.reduce((v,y)=>v+y*y,0)/x.length);}
const flat=rms('OFF',8000);assert(rms('LP',100)<flat*.1);assert(rms('HP',5000)<flat*.03);assert(rms('BP',440)>flat*.8);
const h=decimatorKernel();assert(Math.abs(h.reduce((a,b)=>a+b,0)-1)<1e-12);for(let i=0;i<h.length;i++)assert(Math.abs(h[i]-h[h.length-1-i])<1e-12);
assert.throws(()=>validateParams({invertA:.5}));
for(const rate of [8000,44100,48000,96000]){
 const s=new Synth(rate);s.setBank(bank);for(let i=0;i<8;i++)s.noteOn(24+i*12);
 const b=new Float32Array(2048);
 for(const filter of ['OFF','LP','BP','HP'])for(const cutoff of [20,18000]){s.setParams({filter,cutoff,resonance:1,drive:1,depth:1,ratio:16});s.process(b);assert(b.every(x=>Number.isFinite(x)&&Math.abs(x)<=1));}
 s.panic();s.process(b);s.process(b);assert(b.every(x=>x===0));
}
const s=new Synth();s.setBank(bank);s.noteOn(60);s.process(new Float32Array(5000));s.setParams({target:'MORPH',curve:'HARD',mode:'FREE',free:2000,phaseA:359,invertB:1,filter:'HP',cutoff:20,drive:1});s.process(new Float32Array(1));assert(s.targetMix>0&&s.targetMix<.05);assert(s.curveHard>0&&s.curveHard<.05);assert(Math.abs(s.phaseA)<.001);assert(s.voices[0].cf<2000);s.process(new Float32Array(10000));assert(s.targetMix>.999);assert(Math.abs(s.voices[0].cf-2000)<.001);
console.log('Filter response, sample-rate stress, FIR and parameter slew tests passed');
const env=new Synth();env.setBank(bank);env.setParams({attack:.001,decay:1,sustain:0});env.noteOn(60);env.process(new Float32Array(10000));const oldLevel=env.voices[0].env;env.setParams({sustain:1});env.process(new Float32Array(1));assert(Math.abs(env.voices[0].env-oldLevel)<.01);
