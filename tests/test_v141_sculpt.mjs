import assert from 'node:assert/strict';
import {sculptPreview,SCULPT_MODES} from '../sim/sculpt.mjs';
const original=Array.from({length:128},(_,i)=>Math.round(Math.sin(i*Math.PI/64)*27000));
for(let mode=0;mode<SCULPT_MODES.length;mode++){
 assert.deepEqual(sculptPreview(original,mode,0,73),original);
 const out=sculptPreview(original,mode,91,80);
 assert.equal(original.length,128);assert.equal(out.length,128);
 assert(out.every(x=>Number.isFinite(x)&&x>=-32768&&x<=32767));
 assert(out.some((x,i)=>x!==original[i]),`mode ${mode} must change sound`);
}
assert.deepEqual(sculptPreview(original,3,91,80),sculptPreview(original,3,91,80));
assert.throws(()=>sculptPreview([]),/128/);
console.log('PASS V1.41 browser sculpt preview and modes');
