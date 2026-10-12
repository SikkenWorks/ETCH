export const WAVE_FX_KEYS=Object.freeze(['drive','bitcrush','chorus','delay','reverb','fold','smooth','mix']);
export const defaultWaveFx=Object.freeze({drive:0,bitcrush:0,chorus:0,delay:0,reverb:0,fold:0,smooth:0,mix:1});
export const WAVE_FX_TARGET_RMS=.5;
export const WAVE_FX_PEAK=.891250938;

const clamp=(x,a,b)=>Math.max(a,Math.min(b,x));
const wrap=(i,n)=>((i%n)+n)%n;

export function validateWaveFx(value={}){
 if(value===null||typeof value!=='object'||Array.isArray(value))throw Error('Invalid wave FX');
 const out={...defaultWaveFx};
 for(const key of WAVE_FX_KEYS){
  if(!(key in value))continue;
  const v=value[key];if(typeof v!=='number'||!Number.isFinite(v)||v<0||v>1)throw Error(`Invalid wave FX ${key}`);out[key]=v;
 }
 return out;
}

function baseCycle(points){
 if(!Array.isArray(points)||points.length!==128||points.some(x=>!Number.isInteger(x)||x< -32768||x>32767))throw Error('Wave requires 128 signed 16-bit points');
 const mean=points.reduce((a,b)=>a+b,0)/points.length;
 const values=Float64Array.from(points,x=>(x-mean)/32768);
 let peak=0;for(const x of values)peak=Math.max(peak,Math.abs(x));
 if(peak>0){const s=WAVE_FX_PEAK/peak;for(let i=0;i<values.length;i++)values[i]*=s;}
 return values;
}
function mixStage(source,processed,amount){
 const a=clamp(amount,0,1),out=new Float64Array(source.length);for(let i=0;i<out.length;i++)out[i]=source[i]*(1-a)+processed[i]*a;return out;
}
function normalizeForMorph(values){
 const out=Float64Array.from(values);let mean=0;for(const x of out)mean+=x;mean/=out.length;let rms=0,peak=0;
 for(let i=0;i<out.length;i++){out[i]-=mean;rms+=out[i]*out[i];peak=Math.max(peak,Math.abs(out[i]));}
 rms=Math.sqrt(rms/out.length);if(rms<1e-12)return out;
 let scale=WAVE_FX_TARGET_RMS/rms;if(peak*scale>WAVE_FX_PEAK)scale=WAVE_FX_PEAK/peak;
 for(let i=0;i<out.length;i++)out[i]*=scale;return out;
}
function driveStage(input,amount){const a=clamp(amount,0,1);if(a<=0)return input;const gain=1+11*a,norm=Math.tanh(gain)||1,wet=Float64Array.from(input,x=>Math.tanh(gain*x)/norm);return mixStage(input,wet,a);}
function foldStage(input,amount){const a=clamp(amount,0,1);if(a<=0)return input;const gain=1+5*a,wet=Float64Array.from(input,x=>(2/Math.PI)*Math.asin(Math.sin((Math.PI/2)*gain*x)));return mixStage(input,wet,a);}
function bitcrushStage(input,amount){const a=clamp(amount,0,1);if(a<=0)return input;const bits=Math.max(3,Math.round(16-13*a)),levels=(1<<Math.min(bits,15))-1,hold=1+Math.floor(7*a*a),wet=new Float64Array(input.length);let held=0;for(let i=0;i<input.length;i++){if(i%hold===0){const n=clamp((input[i]+1)*.5,0,1);held=(Math.round(n*levels)/levels)*2-1;}wet[i]=held;}return mixStage(input,wet,a);}
function chorusStage(input,amount){const a=clamp(amount,0,1);if(a<=0)return input;const n=input.length,offset=1+Math.round(7*a),wet=new Float64Array(n);for(let i=0;i<n;i++)wet[i]=.50*input[i]+.25*input[wrap(i-offset,n)]+.25*input[wrap(i+offset,n)];return mixStage(input,wet,.78*a);}
function delayStage(input,amount){const a=clamp(amount,0,1);if(a<=0)return input;const n=input.length,d=Math.max(1,Math.round(n*(.08+.30*a))),wet=new Float64Array(n);for(let i=0;i<n;i++)wet[i]=.62*input[wrap(i-d,n)]+.25*input[wrap(i-2*d,n)]+.13*input[wrap(i-3*d,n)];return mixStage(input,wet,.72*a);}
function reverbStage(input,amount){const a=clamp(amount,0,1);if(a<=0)return input;const n=input.length,offsets=[3,11,23,37,53],weights=[1,.78,.59,.43,.31],sum=weights.reduce((x,y)=>x+y,0),wet=new Float64Array(n);for(let i=0;i<n;i++){let v=0;for(let k=0;k<offsets.length;k++)v+=input[wrap(i-offsets[k],n)]*weights[k];wet[i]=v/sum;}return mixStage(input,wet,.68*a);}
function smoothStage(input,amount){const a=clamp(amount,0,1);if(a<=0)return input;const n=input.length,iterations=1+Math.floor(4*a);let wet=Float64Array.from(input);for(let k=0;k<iterations;k++){const next=new Float64Array(n);for(let i=0;i<n;i++)next[i]=.25*wet[wrap(i-1,n)]+.5*wet[i]+.25*wet[wrap(i+1,n)];wet=next;}return mixStage(input,wet,a);}

// Non-destructive pre-morph waveform FX. Time-style effects are cyclic transforms, so the
// result remains a single-cycle waveform and A/B still morph by shape rather than audio crossfade.
export function applyWaveFx(points,fx={}){
 const p=validateWaveFx(fx),dry=baseCycle(points);let wet=Float64Array.from(dry);
 wet=driveStage(wet,p.drive);wet=foldStage(wet,p.fold);wet=bitcrushStage(wet,p.bitcrush);wet=chorusStage(wet,p.chorus);wet=delayStage(wet,p.delay);wet=reverbStage(wet,p.reverb);wet=smoothStage(wet,p.smooth);
 const mixed=mixStage(dry,wet,p.mix);return normalizeForMorph(mixed);
}
export function waveFxPoints(points,fx={}){return Array.from(applyWaveFx(points,fx),x=>Math.round(clamp(x,-.999969482421875,.999969482421875)*32768));}
export function waveFxRms(points,fx={}){const v=applyWaveFx(points,fx);let s=0;for(const x of v)s+=x*x;return Math.sqrt(s/v.length);}
