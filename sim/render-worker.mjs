import {Synth,compileWave} from './dsp.mjs';
self.onmessage=({data})=>{
 try{
  const sr=44100,s=new Synth(sr);s.setParams(data.params);s.setBank(data.waves.map(w=>compileWave(w.points,w.fx)));
  const on=sr*3,total=on+Math.ceil((data.params.release+.1)*sr),pcm=new Float32Array(total);
  for(const note of data.notes.slice(0,8))s.noteOn(note,.85);
  for(let i=0;i<on;i+=4096)s.process(pcm.subarray(i,Math.min(i+4096,on)));
  for(const note of data.notes)s.noteOff(note);
  for(let i=on;i<total;i+=4096)s.process(pcm.subarray(i,Math.min(i+4096,total)));
  const buffer=new ArrayBuffer(44+total*2),v=new DataView(buffer);
  const text=(offset,s)=>{for(let i=0;i<s.length;i++)v.setUint8(offset+i,s.charCodeAt(i));};
  text(0,'RIFF');v.setUint32(4,36+total*2,true);text(8,'WAVEfmt ');v.setUint32(16,16,true);v.setUint16(20,1,true);v.setUint16(22,1,true);v.setUint32(24,sr,true);v.setUint32(28,sr*2,true);v.setUint16(32,2,true);v.setUint16(34,16,true);text(36,'data');v.setUint32(40,total*2,true);
  for(let i=0;i<total;i++)v.setInt16(44+2*i,Math.round(Math.max(-1,Math.min(1,pcm[i]))*32767),true);
  self.postMessage({buffer},[buffer]);
 }catch(e){self.postMessage({error:e.message});}
};
