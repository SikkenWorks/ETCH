// ETCH original host reference DSP. Floating point; no hardware backend yet.
export const N=128, SIZE=512, HARMONICS=[64,32,16,8,2,1];
export const clamp=(x,a,b)=>Math.max(a,Math.min(b,x));
export function primitive(type){return Array.from({length:N},(_,i)=>{
 const p=i/N; return Math.round(29199*(type==='SINE'?Math.sin(2*Math.PI*p):type==='SAW'?2*p-1:type==='SQUARE'?(p<.5?1:-1):1-4*Math.abs(p-.5)));
});}
export function compileWave(points){
 if(!Array.isArray(points)||points.length!==N||points.some(x=>!Number.isInteger(x)||x< -32768||x>32767))throw Error('Wave requires 128 signed 16-bit points');
 const mean=points.reduce((a,b)=>a+b,0)/N;
 const source=points.map(x=>(x-mean)/32768), peak=Math.max(...source.map(Math.abs));
 const scale=peak>0?.891250938/peak:0;
 const re=new Float64Array(65), im=new Float64Array(65);
 for(let k=1;k<=64;k++)for(let j=0;j<N;j++){
  const a=2*Math.PI*k*j/N; re[k]+=source[j]*scale*Math.cos(a)/N; im[k]-=source[j]*scale*Math.sin(a)/N;
 }
 const levels=HARMONICS.map(h=>{
  const t=new Float32Array(SIZE);
  for(let i=0;i<SIZE;i++)for(let k=1;k<=h;k++){
   const a=2*Math.PI*k*i/SIZE, gain=k===64?1:2;
   t[i]+=gain*(re[k]*Math.cos(a)-im[k]*Math.sin(a));
  }return t;
 });
 let max=0; for(const t of levels)for(const v of t)max=Math.max(max,Math.abs(v));
 if(max>.95)for(const t of levels)for(let i=0;i<SIZE;i++)t[i]*=.95/max;
 return levels;
}
export function readTable(t,p){p=(p-Math.floor(p))*SIZE;const whole=Math.floor(p),i=whole%SIZE,f=p-whole;return t[i]+(t[(i+1)%SIZE]-t[i])*f;}
export function readWave(w,p,f,sr){
 if(f>=sr*.45)return 0;
 const budget=sr*.45/Math.max(f,.001);
 let i=0;while(i<5&&HARMONICS[i]>budget)i++;
 if(i===5)return readTable(w[5],p);
 const mix=clamp((budget/HARMONICS[i]-1),0,1);
 return readTable(w[i+1],p)*(1-mix)+readTable(w[i],p)*mix;
}
export const defaults={morph:.35,depth:0,ratio:1,mode:'KEYED',target:'PHASE',curve:'LIN',free:2,bpm:120,bars:1,attack:.01,decay:.2,sustain:.7,release:.3,gain:.55};
export function validateParams(p){
 if(!p||typeof p!=='object'||Array.isArray(p))throw Error('Invalid parameters');
 const q={...defaults};const bounds={morph:[0,1],depth:[0,1],ratio:[.125,16],free:[.05,2000],bpm:[30,300],bars:[1/64,8],attack:[.001,4],decay:[.001,4],sustain:[0,1],release:[.005,5],gain:[0,1]};
 for(const [k,[lo,hi]]of Object.entries(bounds)){if(k in p){if(typeof p[k]!=='number'||!Number.isFinite(p[k])||p[k]<lo||p[k]>hi)throw Error(`Invalid ${k}`);q[k]=p[k];}}
 for(const [k,values]of Object.entries({mode:['KEYED','FREE','TEMPO'],target:['PHASE','MORPH'],curve:['LIN','S','HARD']})){if(k in p){if(!values.includes(p[k]))throw Error(`Invalid ${k}`);q[k]=p[k];}}
 return q;
}
export class Synth{
 constructor(sr=44100){
  this.sr=sr;this.p={...defaults};this.m=this.p.morph;this.d=0;this.g=this.p.gain;this.age=0;this.bank=null;this.old=null;this.fade=0;this.pending=null;
  this.voices=Array.from({length:8},()=>({note:-1,phase:0,q:0,env:0,stage:0,f:0,vel:0,age:0,last:0,tail:0,steal:0,releaseStep:0}));
 }
 setBank(b){if(!b||b.length!==3||b.some(w=>w.length!==6||w.some(t=>t.length!==SIZE||t.some(x=>!Number.isFinite(x)))))throw Error('Invalid runtime bank');if(this.fade){this.pending=b;return;}this.old=this.bank;this.bank=b;this.fade=this.old?256:0;}
 setParams(p){this.p=validateParams({...this.p,...p});}
 noteOn(note,velocity=1){
  if(!Number.isInteger(note)||note<0||note>127||!Number.isFinite(velocity))return;
  if(velocity<=0){this.noteOff(note);return;}
  let v=this.voices.find(v=>v.stage===0);
  if(!v)v=this.voices.reduce((a,b)=>a.env<b.env?a:b);
  const tail=v.last;Object.assign(v,{note,phase:0,q:0,env:0,stage:1,f:440*2**((note-69)/12),vel:clamp(velocity,0,1),age:++this.age,last:0,tail,steal:64,releaseStep:0});
 }
 noteOff(note){for(const v of this.voices)if(v.note===note&&v.stage&&v.stage!==4){v.stage=4;v.releaseStep=v.env/(this.p.release*this.sr);}}
 panic(){for(const v of this.voices){v.stage=4;v.releaseStep=v.env/64;v.steal=0;v.tail=0;}}
 carrier(bank,v,m,d){
  const p=this.p, cf=p.mode==='KEYED'?v.f*p.ratio:p.mode==='FREE'?p.free:p.bpm/(240*p.bars);
  const c=readWave(bank[2],v.q,cf,this.sr), phase=v.phase+(p.target==='PHASE'?d*.25*c:0);
  let blend=clamp(m+(p.target==='MORPH'?d*c:0),0,1);
  if(p.curve==='S')blend=blend*blend*(3-2*blend);else if(p.curve==='HARD')blend=blend<.5?2*blend*blend:1-2*(1-blend)**2;
  return readWave(bank[0],phase,v.f,this.sr)*(1-blend)+readWave(bank[1],phase,v.f,this.sr)*blend;
 }
 process(out){
  const p=this.p,sr=this.sr,smooth=1-Math.exp(-1/(.005*sr));
  for(let i=0;i<out.length;i++){
   this.m+=(p.morph-this.m)*smooth;this.d+=(p.depth-this.d)*smooth;this.g+=(p.gain-this.g)*smooth;let sum=0;
   for(const v of this.voices){
    if(!v.stage)continue;
    if(v.stage===1){v.env=Math.min(1,v.env+1/(p.attack*sr));if(v.env>=1)v.stage=2;}
    else if(v.stage===2){v.env=Math.max(p.sustain,v.env-(1-p.sustain)/(p.decay*sr));if(v.env<=p.sustain)v.stage=3;}
    else if(v.stage===3)v.env+=(p.sustain-v.env)*smooth;
    else {v.env=Math.max(0,v.env-v.releaseStep);if(!v.env){v.stage=0;v.last=0;continue;}}
    let y=this.bank?this.carrier(this.bank,v,this.m,this.d):0;
    if(this.fade&&this.old){const w=this.fade/256;y=y*(1-w)+this.carrier(this.old,v,this.m,this.d)*w;}
    y*=v.env*v.vel;
    if(v.steal){const w=v.steal--/64;y=y*(1-w)+v.tail*w;}
    v.last=y;sum+=y;
    const cf=p.mode==='KEYED'?v.f*p.ratio:p.mode==='FREE'?p.free:p.bpm/(240*p.bars);
    v.phase=(v.phase+v.f/sr)%1;v.q=(v.q+cf/sr)%1;
   }
   // Fixed 8-voice headroom, so notes cannot pump another note's level.
   out[i]=clamp(sum*this.g/8,-1,1);
   if(this.fade&&!--this.fade){this.old=null;if(this.pending){const b=this.pending;this.pending=null;this.setBank(b);}}
  }
 }
}
