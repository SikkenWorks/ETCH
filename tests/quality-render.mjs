import fs from 'node:fs';
import {Synth,compileWave,primitive} from '../sim/dsp.mjs';
import {Synth as OldSynth,compileWave as oldCompileWave,primitive as oldPrimitive} from './reference-v01.mjs';
const dir=process.argv[2];if(!dir)throw Error('Output directory required');
const scenarios=[
 ['SINE',primitive('SINE'),oldPrimitive('SINE')],
 ['SAW',primitive('SAW'),oldPrimitive('SAW')],
 ['SQUARE',primitive('SQUARE'),oldPrimitive('SQUARE')],
 ['SCRATCH',Array.from({length:128},(_,i)=>Math.round(26000*(.62*Math.sin(i*.41)+.28*Math.sin(i*1.73)+.1*Math.sin(i*2.89)))),Array.from({length:128},(_,i)=>Math.round(26000*(.62*Math.sin(i*.41)+.28*Math.sin(i*1.73)+.1*Math.sin(i*2.89))))]
];
for(const [name,newPoints,oldPoints] of scenarios){
 const newWave=compileWave(newPoints),oldWave=oldCompileWave(oldPoints),newBank=[newWave,newWave,newWave],oldBank=[oldWave,oldWave,oldWave];
 for(const note of [84,96])for(const [tag,Type,bank] of [['v01',OldSynth,oldBank],['v04',Synth,newBank]]){
  const s=new Type(44100);s.setBank(bank);s.setParams({morph:0,depth:1,ratio:8,attack:.001,decay:.001,sustain:1,gain:1});s.noteOn(note);s.process(new Float32Array(44100));const output=new Float32Array(131072);s.process(output);fs.writeFileSync(`${dir}/${name}-${tag}-${note}.f32`,Buffer.from(output.buffer));
 }
}
// A single full-level eight-voice stress block sequence, measured after warmup.
const wave=compileWave(primitive('SINE')),bank=[wave,wave,wave],s=new Synth();s.setBank(bank);s.setParams({depth:1,ratio:8,filter:'LP',cutoff:18000,resonance:1,drive:1});for(let i=0;i<8;i++)s.noteOn(48+i*3);const block=new Float32Array(128);for(let i=0;i<100;i++)s.process(block);const t=performance.now();for(let i=0;i<500;i++)s.process(block);console.log(JSON.stringify({hostAudioSeconds:500*128/44100,hostWallSeconds:(performance.now()-t)/1000,q15ActiveBankBytes:3*6*512*2}));
