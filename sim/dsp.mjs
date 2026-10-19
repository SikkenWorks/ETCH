import {applyWaveFx,defaultWaveFx,validateWaveFx} from './wave-fx.mjs';
// ETCH original host reference DSP. Floating point; no hardware backend yet.
export const N=128, SIZE=512, HARMONICS=[64,32,16,8,2,1], Q15=32768;
export const clamp=(x,a,b)=>Math.max(a,Math.min(b,x));
export function primitive(type){return Array.from({length:N},(_,i)=>{
 const p=i/N; return Math.round(29199*(type==='SINE'?Math.sin(2*Math.PI*p):type==='SAW'?2*p-1:type==='SQUARE'?(p<.5?1:-1):1-4*Math.abs(p-.5)));
});}
export function compileWave(points,fx=defaultWaveFx){
 validateWaveFx(fx);
 const source=applyWaveFx(points,fx);
 const re=new Float64Array(65), im=new Float64Array(65);
 for(let k=1;k<=64;k++)for(let j=0;j<N;j++){
  const a=2*Math.PI*k*j/N; re[k]+=source[j]*Math.cos(a)/N; im[k]-=source[j]*Math.sin(a)/N;
 }
 const floating=HARMONICS.map(h=>{
  const t=new Float64Array(SIZE);
  for(let i=0;i<SIZE;i++)for(let k=1;k<=h;k++){
   const a=2*Math.PI*k*i/SIZE, gain=k===64?1:2;
   t[i]+=gain*(re[k]*Math.cos(a)-im[k]*Math.sin(a));
  }return t;
 });
 let max=0; for(const t of floating)for(const v of t)max=Math.max(max,Math.abs(v));
 const limiter=max>.95?.95/max:1;
 return floating.map(source=>{const t=new Int16Array(SIZE);for(let i=0;i<SIZE;i++)t[i]=Math.round(clamp(source[i]*limiter,-.999969482421875,.999969482421875)*Q15);return t;});
}
export function readTable(t,p){p=(p-Math.floor(p))*SIZE;const whole=Math.floor(p),i=whole%SIZE,f=p-whole;return (t[i]+(t[(i+1)%SIZE]-t[i])*f)/Q15;}
export function readWave(w,p,f,sr){
 if(f>=sr*.45)return 0;
 const budget=sr*.45/Math.max(f,.001);
 let i=0;while(i<5&&HARMONICS[i]>budget)i++;
 if(i===5)return readTable(w[5],p)*clamp((sr*.45-f)/(sr*.05),0,1);
 const mix=clamp((budget/HARMONICS[i]-1),0,1);
 return readTable(w[i+1],p)*(1-mix)+readTable(w[i],p)*mix;
}
// Matches the native ETCH Q7 velocity curve at MIDI precision.
export function expressiveVelocity(velocity, curve='LINEAR'){
 const v=Math.max(0,Math.min(127,Math.round(clamp(velocity,0,1)*127)));
 if(curve==='SOFT')return Math.floor((v*(254-v)+126)/127)/127;
 if(curve==='HARD')return Math.floor((v*v+63)/127)/127;
 return clamp(velocity,0,1); // exact legacy browser response for LINEAR
}
export const MATRIX_SOURCES=['OFF','VELOCITY','ENVELOPE','WAVE C','LIVING','AFTERTOUCH','KEYTRACK'];
export const MATRIX_TARGETS=['MORPH','C DEPTH','FILTER','SPACE'];
export const emptyModMatrix=()=>Array.from({length:4},()=>({source:0,target:0,amount:0}));
export function validateModMatrix(routes){
 if(!Array.isArray(routes)||routes.length!==4)throw Error('Invalid Mod Matrix routes');
 return routes.map(r=>{
  if(!r||!Number.isInteger(r.source)||r.source<0||r.source>6||
    !Number.isInteger(r.target)||r.target<0||r.target>3||
    !Number.isInteger(r.amount)||r.amount< -100||r.amount>100)throw Error('Invalid Mod Matrix route');
  return {source:r.source,target:r.target,amount:r.amount};
 });
}
export function modMatrixValue(routes,target,base,velocity,env,waveC,living,aftertouch,note){
 let value=base;
 for(const r of routes){if(!r.source||!r.amount||r.target!==target)continue;
  const source=[0,velocity,env,waveC,living,aftertouch,note/127][r.source];
  value+=source*r.amount/400;
 }
 return clamp(value,0,1);
}
export const defaults={morph:.35,depth:0,ratio:1,mode:'KEYED',target:'PHASE',curve:'LIN',free:2,bpm:120,bars:1,attack:.01,decay:.2,sustain:.7,release:.3,fAttack:.01,fDecay:.2,fSustain:.7,fRelease:.3,filterEnv:0,gain:.55,phaseA:0,phaseB:0,invertA:0,invertB:0,filter:'OFF',cutoff:8000,resonance:0,drive:0,chorus:0,delay:0,reverb:0,voice:'MONO',glide:0,bendRange:2,velSense:1,velocityCurve:'LINEAR',modMatrix:emptyModMatrix()};
export function validateParams(p){
 if(!p||typeof p!=='object'||Array.isArray(p))throw Error('Invalid parameters');
 const q={...defaults,modMatrix:validateModMatrix(p.modMatrix??emptyModMatrix())};const bounds={morph:[0,1],depth:[0,1],ratio:[.125,16],free:[.05,2000],bpm:[30,300],bars:[1/64,8],attack:[.001,4],decay:[.001,4],sustain:[0,1],release:[.005,5],fAttack:[.001,4],fDecay:[.001,4],fSustain:[0,1],fRelease:[.005,5],filterEnv:[-1,1],gain:[0,1],phaseA:[0,359],phaseB:[0,359],invertA:[0,1],invertB:[0,1],cutoff:[20,18000],resonance:[0,1],drive:[0,1],chorus:[0,1],delay:[0,1],reverb:[0,1],glide:[0,2],bendRange:[1,12],velSense:[0,1]};
 for(const [k,[lo,hi]]of Object.entries(bounds)){if(k in p){if(typeof p[k]!=='number'||!Number.isFinite(p[k])||p[k]<lo||p[k]>hi)throw Error(`Invalid ${k}`);q[k]=p[k];}}
 for(const [k,values]of Object.entries({mode:['KEYED','FREE','TEMPO'],target:['PHASE','MORPH'],curve:['LIN','S','HARD'],filter:['OFF','LP','BP','HP'],voice:['POLY','MONO'],velocityCurve:['LINEAR','SOFT','HARD']})){if(k in p){if(!values.includes(p[k]))throw Error(`Invalid ${k}`);q[k]=p[k];}}
 if(![0,1].includes(q.invertA)||![0,1].includes(q.invertB))throw Error('Invert must be 0 or 1');
 return q;
}
export const OVERSAMPLE=4, FIR_TAPS=127;
export function decimatorKernel(){
 const h=new Float64Array(FIR_TAPS);let sum=0;
 for(let i=0;i<FIR_TAPS;i++){
  const t=i-63,fc=.115;
  const sinc=t===0?2*fc:Math.sin(2*Math.PI*fc*t)/(Math.PI*t);
  const window=.42-.5*Math.cos(2*Math.PI*i/126)+.08*Math.cos(4*Math.PI*i/126);
  h[i]=sinc*window;sum+=h[i];
 }
 for(let i=0;i<FIR_TAPS;i++)h[i]/=sum;
 return h;
}
// UI-only static oscillator view. Never construct an audio engine to draw a frame.
export function previewSample(bank,p,phase,noteHz=261.626){
 const cf=p.mode==='KEYED'?noteHz*p.ratio:p.mode==='FREE'?p.free:p.bpm/(240*p.bars);
 const c=readWave(bank[2],phase*cf/noteHz,cf,44100*OVERSAMPLE);
 let m=clamp(p.morph+(p.target==='MORPH'?p.depth*c:0),0,1);
 if(p.curve==='S')m=m*m*(3-2*m);else if(p.curve==='HARD')m=m<.5?2*m*m:1-2*(1-m)**2;
 const q=phase+(p.target==='PHASE'?p.depth*.25*c:0);
 return readWave(bank[0],q+p.phaseA/360,noteHz,44100*OVERSAMPLE)*(1-2*p.invertA)*(1-m)+readWave(bank[1],q+p.phaseB/360,noteHz,44100*OVERSAMPLE)*(1-2*p.invertB)*m;
}
export class Synth{
 constructor(sr=44100){
  if(!Number.isFinite(sr)||sr<8000||sr>192000)throw Error('Invalid sample rate');
  this.sr=sr;this.rate=sr*OVERSAMPLE;this.smooth=1-Math.exp(-1/(.005*this.rate));this.morphSmooth=1-Math.exp(-1/(.015*this.rate));this.fxSmooth=1-Math.exp(-1/(.02*this.sr));
  this.p={...defaults,modMatrix:emptyModMatrix()};this.sustain=this.p.sustain;this.m=this.p.morph;this.d=0;this.g=this.p.gain;
  this.targetMix=0;this.curveS=0;this.curveHard=0;this.phaseA=0;this.phaseB=0;this.invA=1;this.invB=1;
  this.filterEnvLut=new Float64Array(257);this.filterFreqLut=new Float64Array(2049);this.filterMax=Math.min(18000,.4*this.sr);this.damp=2;this.drive=0;this.lp=0;this.bp=0;this.hp=0;this.chorus=0;this.delay=0;this.reverb=0;this.chorusPhase=0;
  this.glideCoeff=1;this.bendNorm=0;this.bendRatio=1;this.bendRatioTarget=1;this.externalBpm=null;this.monoStack=[];this.monoVelocity=new Float32Array(128);
  this.matrixSpace=0;this.channelPressure=new Float64Array(16);this.bank=null;this.old=null;this.fade=0;this.pending=null;this.kernel=decimatorKernel();this.history=new Float64Array(128);this.head=0;
  const length=seconds=>Math.max(8,Math.ceil(this.sr*seconds)+4);this.chorusBuffer=new Float64Array(length(.04));this.delayBuffer=new Float64Array(length(.26));this.chorusHead=0;this.delayHead=0;this.comb=[.0297,.0371,.0411,.0437].map(t=>({buffer:new Float64Array(length(t)),head:0}));this.allpass=[.005,.0017].map(t=>({buffer:new Float64Array(length(t)),head:0}));this.fxTail=0;
  this.voices=Array.from({length:8},()=>({note:-1,id:0,channel:-1,pressure:0,phase:0,q:0,cf:0,env:0,stage:0,fenv:0,fstage:0,f:0,baseF:0,targetF:0,vel:0,last:0,tail:0,steal:0,releaseStep:0,fReleaseStep:0,ic1:0,ic2:0}));
  this.rebuildFilterTables();
 }
 rebuildFilterTables(){for(let i=0;i<this.filterFreqLut.length;i++){const cutoff=20+(this.filterMax-20)*i/(this.filterFreqLut.length-1);this.filterFreqLut[i]=Math.tan(Math.PI*cutoff/this.rate);}this.rebuildFilterEnvLut();}
 rebuildFilterEnvLut(){for(let i=0;i<this.filterEnvLut.length;i++)this.filterEnvLut[i]=2**(5*this.p.filterEnv*(i/256));}
 filterG(env){const x=clamp(env,0,1)*256,ei=Math.min(255,Math.floor(x)),ef=x-ei,mul=this.filterEnvLut[ei]+(this.filterEnvLut[ei+1]-this.filterEnvLut[ei])*ef,cutoff=clamp(this.p.cutoff*mul,20,this.filterMax),pos=(cutoff-20)/(this.filterMax-20)*(this.filterFreqLut.length-1),i=Math.min(this.filterFreqLut.length-2,Math.floor(pos)),f=pos-i;return this.filterFreqLut[i]+(this.filterFreqLut[i+1]-this.filterFreqLut[i])*f;}
 setBank(b){if(!b||b.length!==3||b.some(w=>w.length!==6||w.some(t=>t.length!==SIZE||t.some(x=>!Number.isFinite(x)))))throw Error('Invalid runtime bank');if(this.fade){this.pending=b;return;}this.old=this.bank;this.bank=b;this.fade=this.old?256*OVERSAMPLE:0;}
 setParams(p){const prior=this.p,priorVoice=prior.voice,next=validateParams({...prior,...p});this.p=next;if(next.filterEnv!==prior.filterEnv)this.rebuildFilterEnvLut();if(next.glide!==prior.glide)this.glideCoeff=next.glide<=0?1:1-Math.exp(-1/(next.glide*this.rate));if(next.bendRange!==prior.bendRange)this.bendRatioTarget=2**(this.bendNorm*next.bendRange/12);if(priorVoice!==next.voice){this.monoStack.length=0;this.monoVelocity.fill(0);this.panic();}}
 applyPerformanceLocks(base,locks){this.p.morph=locks.morph??base.morph;this.p.depth=locks.depth??base.depth;this.p.cutoff=locks.cutoff??base.cutoff;this.p.chorus=locks.chorus??base.chorus;this.p.delay=locks.delay??base.delay;this.p.reverb=locks.reverb??base.reverb;}
 modFrequency(v){const p=this.p,bpm=this.externalBpm??p.bpm;return p.mode==='KEYED'?v.f*p.ratio:p.mode==='FREE'?p.free:bpm/(240*p.bars);}
 setExternalTempo(bpm=null){if(bpm===null){this.externalBpm=null;return true;}if(!Number.isFinite(bpm)||bpm<30||bpm>300)return false;this.externalBpm=bpm;return true;}
 activeVoices(){return this.voices.reduce((n,v)=>n+(v.stage?1:0),0);}
 noteHz(note){return 440*2**((note-69)/12);}
 monoRemove(note){const i=this.monoStack.indexOf(note);if(i>=0)this.monoStack.splice(i,1);this.monoVelocity[note]=0;}
 noteOn(note,velocity=1,instance=0,channel=-1){
  if(!Number.isInteger(channel)||channel<-1||channel>15)return;
  if(!Number.isInteger(note)||note<0||note>127||!Number.isFinite(velocity))return;
  const id=Number.isInteger(instance)&&instance>0?instance:0;
  if(this.p.chorus>0||this.p.delay>0||this.p.reverb>0)this.fxTail=Math.max(this.fxTail,Math.floor(this.sr*4));
  if(velocity<=0){this.noteOff(note,instance);return;}
  const effective=clamp(1-this.p.velSense+this.p.velSense*expressiveVelocity(velocity,this.p.velocityCurve),0,1);
  if(this.p.voice==='MONO'){
   const hadHeld=this.monoStack.length>0;this.monoRemove(note);this.monoStack.push(note);this.monoVelocity[note]=effective;
   const v=this.voices[0];for(let i=1;i<this.voices.length;i++)Object.assign(this.voices[i],{note:-1,id:0,channel:-1,pressure:0,phase:0,q:0,cf:0,env:0,stage:0,fenv:0,fstage:0,f:0,baseF:0,targetF:0,vel:0,last:0,tail:0,steal:0,releaseStep:0,fReleaseStep:0,ic1:0,ic2:0});
   if(!v.stage||!hadHeld){const tail=v.last,env=v.env;Object.assign(v,{note,id,channel,pressure:channel<0?0:this.channelPressure[channel],phase:0,q:0,cf:0,env,stage:1,fenv:0,fstage:1,f:this.noteHz(note),baseF:this.noteHz(note),targetF:this.noteHz(note),vel:effective,last:0,tail,steal:64*OVERSAMPLE,releaseStep:0,fReleaseStep:0,ic1:0,ic2:0});v.cf=this.modFrequency(v);}
   else {v.note=note;v.id=id;v.channel=channel;v.pressure=channel<0?0:this.channelPressure[channel];v.targetF=this.noteHz(note);v.vel=effective;}
   return;
  }
  let v=this.voices.find(v=>v.stage===0);
  if(!v)v=this.voices.reduce((a,b)=>a.env<b.env?a:b);
  const f=this.noteHz(note),tail=v.last;Object.assign(v,{note,id,channel,pressure:channel<0?0:this.channelPressure[channel],phase:0,q:0,env:0,stage:1,fenv:0,fstage:1,f,baseF:f,targetF:f,vel:effective,last:0,tail,steal:64*OVERSAMPLE,releaseStep:0,fReleaseStep:0,ic1:0,ic2:0});v.cf=this.modFrequency(v);
 }
 noteOff(note,instance=0){const id=Number.isInteger(instance)&&instance>0?instance:0;if(this.p.voice==='MONO'){this.monoRemove(note);const v=this.voices[0];if(this.monoStack.length){const next=this.monoStack.at(-1);v.note=next;v.targetF=this.noteHz(next);v.vel=this.monoVelocity[next];}else if(v.stage&&v.stage!==4){v.stage=4;v.releaseStep=v.env/(this.p.release*this.rate);v.fstage=4;v.fReleaseStep=v.fenv/(this.p.fRelease*this.rate);}return;}for(const v of this.voices)if(v.note===note&&(!id||v.id===id)&&v.stage&&v.stage!==4){v.stage=4;v.releaseStep=v.env/(this.p.release*this.rate);v.fstage=4;v.fReleaseStep=v.fenv/(this.p.fRelease*this.rate);}}
 setChannelPressure(channel,value){
  if(!Number.isInteger(channel)||channel<0||channel>15||!Number.isFinite(value))return false;
  this.channelPressure[channel]=clamp(value,0,1);return true;
 }
 setPitchBend(normalized){if(!Number.isFinite(normalized))return;this.bendNorm=clamp(normalized,-1,1);this.bendRatioTarget=2**(this.bendNorm*this.p.bendRange/12);}
 panic(){this.channelPressure.fill(0);this.monoStack.length=0;this.monoVelocity.fill(0);for(const v of this.voices)Object.assign(v,{note:-1,id:0,channel:-1,pressure:0,phase:0,q:0,cf:0,env:0,stage:0,fenv:0,fstage:0,f:0,baseF:0,targetF:0,vel:0,last:0,tail:0,steal:0,releaseStep:0,fReleaseStep:0,ic1:0,ic2:0});this.history.fill(0);this.head=0;this.clearEffects();}
 carrier(bank,v,m=this.m,d=this.d){
  const c=readWave(bank[2],v.q,v.cf,this.rate);
  const matrix=this.p.modMatrix;
  if(matrix.some(r=>r.source&&r.amount)){
   const living=(1+Math.sin(v.phase*Math.PI*2))*.5;
   m=modMatrixValue(matrix,0,m,v.vel,v.env,c,living,v.pressure,v.note);
   d=modMatrixValue(matrix,1,d,v.vel,v.env,c,living,v.pressure,v.note);
   if(v===this.voices.find(q=>q.stage))this.matrixSpace=modMatrixValue(matrix,3,this.p.reverb,v.vel,v.env,c,living,v.pressure,v.note);
  }
  const phase=v.phase+d*.25*c*(1-this.targetMix);
  let blend=clamp(m+(v.pressure??0)*.125+d*c*this.targetMix,0,1);
  const s=blend*blend*(3-2*blend),hard=blend<.5?2*blend*blend:1-2*(1-blend)**2;
  blend=blend*(1-this.curveS-this.curveHard)+s*this.curveS+hard*this.curveHard;
  return readWave(bank[0],phase+this.phaseA,v.f,this.rate)*this.invA*(1-blend)+readWave(bank[1],phase+this.phaseB,v.f,this.rate)*this.invB*blend;
 }
 readDelay(buffer,head,delaySamples){let pos=head-delaySamples,size=buffer.length;while(pos<0)pos+=size;while(pos>=size)pos-=size;const i=Math.floor(pos),j=(i+1)%size,f=pos-i;return buffer[i]+(buffer[j]-buffer[i])*f;}
 clearEffects(){this.chorusBuffer.fill(0);this.delayBuffer.fill(0);this.chorusHead=0;this.delayHead=0;this.chorusPhase=0;this.fxTail=0;for(const line of this.comb){line.buffer.fill(0);line.head=0;}for(const line of this.allpass){line.buffer.fill(0);line.head=0;}}
 processEffects(input){
  this.chorus+=(this.p.chorus-this.chorus)*this.fxSmooth;this.delay+=(this.p.delay-this.delay)*this.fxSmooth;this.reverb+=((this.p.modMatrix.some(r=>r.target===3&&r.source&&r.amount)?this.matrixSpace:this.p.reverb)-this.reverb)*this.fxSmooth;
  if(Math.abs(input)>1e-8&&(this.chorus>1e-5||this.delay>1e-5||this.reverb>1e-5))this.fxTail=Math.max(this.fxTail,Math.floor(this.sr*4));
  let x=input;this.chorusBuffer[this.chorusHead]=x;const lfo=.5+.5*Math.sin(2*Math.PI*this.chorusPhase);this.chorusPhase+=.35/this.sr;if(this.chorusPhase>=1)this.chorusPhase-=1;
  const chorusSamples=this.sr*(.012+.010*lfo),chorused=this.readDelay(this.chorusBuffer,this.chorusHead,chorusSamples);this.chorusHead=(this.chorusHead+1)%this.chorusBuffer.length;x+=this.chorus*.35*(chorused-x);
  const delayed=this.readDelay(this.delayBuffer,this.delayHead,this.sr*.22);this.delayBuffer[this.delayHead]=x+delayed*(.18+.42*this.delay);this.delayHead=(this.delayHead+1)%this.delayBuffer.length;x+=delayed*(.50*this.delay);
  let wet=0,feedback=.72+.18*this.reverb,rvIn=x*.24;for(const line of this.comb){const y=line.buffer[line.head];line.buffer[line.head]=rvIn+y*feedback;line.head=(line.head+1)%line.buffer.length;wet+=y;}wet*=.25;
  for(const line of this.allpass){const b=line.buffer[line.head],y=-wet+b;line.buffer[line.head]=wet+b*.5;line.head=(line.head+1)%line.buffer.length;wet=y;}x+=wet*(.42*this.reverb);
  if(this.fxTail>0)this.fxTail--;const a=Math.abs(x);if(a>.92){const over=(a-.92)/.08;x=Math.sign(x)*(.92+.08*(over/(1+over)));}return clamp(x,-1,1);
 }
 process(out){
  if(!this.fade&&this.voices.every(v=>!v.stage)&&this.fxTail===0&&this.history.every(x=>x===0)){out.fill(0);return;}
  const p=this.p,sr=this.rate,z=this.smooth;
  for(let i=0;i<out.length;i++){
   for(let os=0;os<OVERSAMPLE;os++){
    this.sustain+=(p.sustain-this.sustain)*z;this.m+=(p.morph-this.m)*this.morphSmooth;this.d+=(p.depth-this.d)*z;this.g+=(p.gain-this.g)*z;this.bendRatio+=(this.bendRatioTarget-this.bendRatio)*z;
    this.targetMix+=((p.target==='MORPH'?1:0)-this.targetMix)*z;this.curveS+=((p.curve==='S'?1:0)-this.curveS)*z;this.curveHard+=((p.curve==='HARD'?1:0)-this.curveHard)*z;
    let da=p.phaseA/360-this.phaseA;da-=Math.floor(da+.5);this.phaseA+=da*z;
    let db=p.phaseB/360-this.phaseB;db-=Math.floor(db+.5);this.phaseB+=db*z;
    this.invA+=(1-2*p.invertA-this.invA)*z;this.invB+=(1-2*p.invertB-this.invB)*z;
    this.damp+=(2-1.8*p.resonance-this.damp)*z;this.drive+=(p.drive-this.drive)*z;
    this.lp+=((p.filter==='LP'?1:0)-this.lp)*z;this.bp+=((p.filter==='BP'?1:0)-this.bp)*z;this.hp+=((p.filter==='HP'?1:0)-this.hp)*z;
    let sum=0;
    for(const v of this.voices){
     if(!v.stage)continue;
     if(this.glideCoeff>=1)v.baseF=v.targetF;else v.baseF+=(v.targetF-v.baseF)*this.glideCoeff;v.f=v.baseF*this.bendRatio;
     if(v.stage===1){v.env=Math.min(1,v.env+1/(p.attack*sr));if(v.env>=1)v.stage=2;}
     else if(v.stage===2){v.env=Math.max(this.sustain,v.env-(1-this.sustain)/(p.decay*sr));if(v.env<=this.sustain)v.stage=3;}
     else if(v.stage===3)v.env+=(this.sustain-v.env)*z;
     else {v.env=Math.max(0,v.env-v.releaseStep);if(!v.env){v.stage=0;v.fstage=0;v.fenv=0;v.last=0;v.ic1=0;v.ic2=0;continue;}}
     if(v.fstage===1){v.fenv=Math.min(1,v.fenv+1/(p.fAttack*sr));if(v.fenv>=1)v.fstage=2;}
     else if(v.fstage===2){v.fenv=Math.max(p.fSustain,v.fenv-(1-p.fSustain)/(p.fDecay*sr));if(v.fenv<=p.fSustain)v.fstage=3;}
     else if(v.fstage===3)v.fenv+=(p.fSustain-v.fenv)*z;
     else if(v.fstage===4)v.fenv=Math.max(0,v.fenv-v.fReleaseStep);
     v.cf+=(this.modFrequency(v)-v.cf)*z;
     if(v.channel>=0){const aim=this.channelPressure[v.channel];v.pressure+=(aim-v.pressure)*this.morphSmooth;}
     let y=this.bank?this.carrier(this.bank,v):0;
     if(this.fade&&this.old){const w=this.fade/(256*OVERSAMPLE);y=y*(1-w)+this.carrier(this.old,v)*w;}
     // TPT state variable filter. Run while bypassed so switches use warm state.
     const hasFilterRoute=p.modMatrix.some(r=>r.target===2&&r.source&&r.amount);
     const matrixFc=hasFilterRoute&&this.bank?modMatrixValue(p.modMatrix,2,p.cutoff/this.filterMax,v.vel,v.env,readWave(this.bank[2],v.q,v.cf,this.rate),(1+Math.sin(v.phase*Math.PI*2))*.5,v.pressure,v.note)*this.filterMax:null;
     const fg=matrixFc===null?this.filterG(v.fenv):Math.tan(Math.PI*clamp(matrixFc,20,this.filterMax)/this.rate),a1=1/(1+fg*(fg+this.damp)),a2=fg*a1,a3=fg*a2;
     const v3=y-v.ic2,v1=a1*v.ic1+a2*v3,v2=v.ic2+a2*v.ic1+a3*v3;
     v.ic1=2*v1-v.ic1;v.ic2=2*v2-v.ic2;
     const hp=y-this.damp*v1-v2;
     y=y*(1-this.lp-this.bp-this.hp)+v2*this.lp+v1*this.damp*this.bp+hp*this.hp;
     // Bounded saturation blends smoothly from true bypass; run before envelope.
     const driven=y*(1+7*this.drive),sat=driven/(1+Math.abs(driven));y=y*(1-this.drive)+sat*this.drive;
     y*=v.env*v.vel;
     if(v.steal){const w=v.steal--/(64*OVERSAMPLE);y=y*(1-w)+v.tail*w;}
     v.last=y;sum+=y;v.phase=(v.phase+v.f/sr)%1;v.q=(v.q+v.cf/sr)%1;
    }
    this.history[this.head]=sum*this.g/8;this.head=(this.head+1)&127;
    if(this.fade&&!--this.fade){this.old=null;if(this.pending){const b=this.pending;this.pending=null;this.old=this.bank;this.bank=b;this.fade=256*OVERSAMPLE;}}
   }
   let y=0;for(let k=0;k<FIR_TAPS;k++)y+=this.kernel[k]*this.history[(this.head-1-k)&127];out[i]=this.processEffects(y);
  }
 }
}
