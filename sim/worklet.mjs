import {Synth} from './dsp.mjs';
import {PerformanceScheduler} from './performance.mjs';
class EtchProcessor extends AudioWorkletProcessor{
 constructor(){super();this.synth=new Synth(sampleRate);this.groups=[new Synth(sampleRate),new Synth(sampleRate)];this.groupScratch=[new Float32Array(128),new Float32Array(128)];this.groupEnabled=[false,false];this.performance=new PerformanceScheduler(sampleRate);this.performance.setBaseParams(this.synth.p);this.telemetryBlocks=0;this.telemetryPeak=0;this.port.onmessage=({data:d})=>{
  try{
   if(d.type==='bank')this.synth.setBank(d.bank);
   else if(d.type==='groupConfig'){for(let i=0;i<2;i++){this.groupEnabled[i]=!!d.groups[i];if(d.groups[i]){this.groups[i].setParams(d.groups[i].params);this.groups[i].setBank(d.groups[i].bank);}else this.groups[i].panic();}}
   else if(d.type==='groupOn'){this.groups[d.group-1]?.noteOn(d.note,d.velocity,0,d.channel??-1);}
   else if(d.type==='groupOff'){this.groups[d.group-1]?.noteOff(d.note);}
   else if(d.type==='params'){this.synth.setParams(d.params);this.performance.setBaseParams(this.synth.p);}
   else if(d.type==='performance')this.performance.setPerformance(d.performance);
   else if(d.type==='on')this.synth.noteOn(d.note,d.velocity,0,d.channel??-1);
   else if(d.type==='off')this.synth.noteOff(d.note);
   else if(d.type==='heldOn')this.performance.heldOn(d.note,d.velocity);
   else if(d.type==='heldOff')this.performance.heldOff(d.note);
   else if(d.type==='transport'){if(d.action==='start'){this.performance.start(d.reset!==false);this.port.postMessage({type:'seq-led',notes:[]});}else if(d.action==='stop'){this.performance.stop(this.synth);this.port.postMessage({type:'seq-led',notes:[]});}}
   else if(d.type==='aftertouch'){this.synth.setChannelPressure(d.channel,d.value);for(const g of this.groups)g.setChannelPressure(d.channel,d.value);}
   else if(d.type==='bend'){this.synth.setPitchBend(d.value);for(const g of this.groups)g.setPitchBend(d.value);}
   else if(d.type==='clock')this.synth.setExternalTempo(d.bpm??null);
   else if(d.type==='panic'){for(const g of this.groups)g.panic();this.performance.stop(this.synth);this.performance.held.clear();this.performance.heldOrder.length=0;this.synth.panic();this.port.postMessage({type:'seq-led',notes:[]});}
  }catch(e){this.port.postMessage({error:e.message});}
 };}
 process(_,outputs){
  const out=outputs[0][0],transport=this.performance.process(this.synth,out);
  // Keep the existing eight-voice base engine intact; two browser parts render independently.
  for(let i=0;i<2;i++){if(!this.groupEnabled[i])continue;const scratch=this.groupScratch[i];if(scratch.length!==out.length)this.groupScratch[i]=new Float32Array(out.length);const tmp=this.groupScratch[i];tmp.fill(0);this.groups[i].process(tmp);for(let k=0;k<out.length;k++)out[k]+=tmp[k];}
  for(let k=0;k<out.length;k++)out[k]=Math.max(-1,Math.min(1,out[k]));
  if(Number.isInteger(transport.triggeredStep))this.port.postMessage({type:'seq-step',seqStep:transport.triggeredStep});
  if(transport.seqLedChanged)this.port.postMessage({type:'seq-led',notes:transport.seqLedNotes});
  let peak=0;for(const x of out)peak=Math.max(peak,Math.abs(x));this.telemetryPeak=Math.max(this.telemetryPeak,peak);
  if(++this.telemetryBlocks>=16){this.port.postMessage({type:'telemetry',voices:this.synth.activeVoices(),peak:this.telemetryPeak,seqPlaying:transport.seqPlaying,seqStep:transport.seqStep});this.telemetryBlocks=0;this.telemetryPeak=0;}
  return true;
 }
}
registerProcessor('etch',EtchProcessor);
