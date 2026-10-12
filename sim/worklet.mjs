import {Synth} from './dsp.mjs';
import {PerformanceScheduler} from './performance.mjs';
class EtchProcessor extends AudioWorkletProcessor{
 constructor(){super();this.synth=new Synth(sampleRate);this.performance=new PerformanceScheduler(sampleRate);this.performance.setBaseParams(this.synth.p);this.telemetryBlocks=0;this.telemetryPeak=0;this.port.onmessage=({data:d})=>{
  try{
   if(d.type==='bank')this.synth.setBank(d.bank);
   else if(d.type==='params'){this.synth.setParams(d.params);this.performance.setBaseParams(this.synth.p);}
   else if(d.type==='performance')this.performance.setPerformance(d.performance);
   else if(d.type==='on')this.synth.noteOn(d.note,d.velocity);
   else if(d.type==='off')this.synth.noteOff(d.note);
   else if(d.type==='heldOn')this.performance.heldOn(d.note,d.velocity);
   else if(d.type==='heldOff')this.performance.heldOff(d.note);
   else if(d.type==='transport'){if(d.action==='start'){this.performance.start(d.reset!==false);this.port.postMessage({type:'seq-led',notes:[]});}else if(d.action==='stop'){this.performance.stop(this.synth);this.port.postMessage({type:'seq-led',notes:[]});}}
   else if(d.type==='bend')this.synth.setPitchBend(d.value);
   else if(d.type==='clock')this.synth.setExternalTempo(d.bpm??null);
   else if(d.type==='panic'){this.performance.stop(this.synth);this.performance.held.clear();this.performance.heldOrder.length=0;this.synth.panic();this.port.postMessage({type:'seq-led',notes:[]});}
  }catch(e){this.port.postMessage({error:e.message});}
 };}
 process(_,outputs){
  const out=outputs[0][0],transport=this.performance.process(this.synth,out);
  if(Number.isInteger(transport.triggeredStep))this.port.postMessage({type:'seq-step',seqStep:transport.triggeredStep});
  if(transport.seqLedChanged)this.port.postMessage({type:'seq-led',notes:transport.seqLedNotes});
  let peak=0;for(const x of out)peak=Math.max(peak,Math.abs(x));this.telemetryPeak=Math.max(this.telemetryPeak,peak);
  if(++this.telemetryBlocks>=16){this.port.postMessage({type:'telemetry',voices:this.synth.activeVoices(),peak:this.telemetryPeak,seqPlaying:transport.seqPlaying,seqStep:transport.seqStep});this.telemetryBlocks=0;this.telemetryPeak=0;}
  return true;
 }
}
registerProcessor('etch',EtchProcessor);
