import assert from 'node:assert/strict';
import {expressiveVelocity,validateParams,defaults,Synth,compileWave,primitive} from '../sim/dsp.mjs';
import {validateProject,encode,decode} from '../sim/storage.mjs';
for(let n=0;n<=127;n++){
 const v=n/127;
 assert.equal(expressiveVelocity(v,'LINEAR'),v);
 assert.equal(expressiveVelocity(v,'SOFT'),Math.floor((n*(254-n)+126)/127)/127);
 assert.equal(expressiveVelocity(v,'HARD'),Math.floor((n*n+63)/127)/127);
}
assert.equal(validateParams({...defaults,velocityCurve:'HARD'}).velocityCurve,'HARD');
assert.throws(()=>validateParams({...defaults,velocityCurve:'INVALID'}));
const legacy=validateParams({gain:.5});assert.equal(legacy.velocityCurve,'LINEAR');
const bank=[compileWave(primitive('SINE')),compileWave(primitive('SINE')),compileWave(primitive('SINE'))];
let levels=[];
for(const velocityCurve of ['LINEAR','SOFT','HARD']){
 const synth=new Synth();synth.setBank(bank);synth.setParams({velocityCurve});synth.noteOn(60,40/127);levels.push(synth.voices.find(v=>v.stage).vel);
}
assert(levels[1]>levels[0] && levels[0]>levels[2]);
const p={version:11,waves:Array(64).fill(null),patches:Array(32).fill(null),projects:Array(16).fill(null),patterns:Array(32).fill(null)};
p.waves[0]={name:'SINE',points:primitive('SINE')};p.patches[0]={name:'SOFT',slots:[0,0,0],params:{...defaults,velocityCurve:'SOFT'}};
const restored=decode(encode(p));assert.equal(restored.patches[0].params.velocityCurve,'SOFT');
const old=structuredClone(p);delete old.patches[0].params.velocityCurve;
assert.equal(validateProject(old).patches[0].params.velocityCurve,'LINEAR');
console.log('PASS velocity curves, exact native Q7 mapping, DSP, patch roundtrip and legacy migration');
