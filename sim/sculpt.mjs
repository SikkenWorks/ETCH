/* ETCH V1.41. Read-only preview, commit through existing 16-state Undo / wave save. */
export const SCULPT_MODES=Object.freeze(['FOLD','BEND','ASYM','PHASE','HARMONICS']);
const limit=(x,a,b)=>Math.min(b,Math.max(a,x));
export function sculptPreview(source,mode=0,amount=0,pivot=50){
 if(!source || source.length!==128)throw Error('SCULPT requires 128 samples');
 const out=Array.from(source);
 const a=limit(+amount||0,0,100)/100,p=limit(+pivot||0,0,100),anchor=16+Math.floor(p*96/100);
 const sample=idx=>{idx=limit(idx,0,127);const i=Math.floor(idx),frac=idx-i;return source[i]*(1-frac)+source[(i+1)%128]*frac};
 if(!a||!SCULPT_MODES.includes(SCULPT_MODES[mode]))return out;
 for(let i=0;i<128;i++){
  const dry=source[i];let wet=dry;
  if(mode===0){let x=dry*(1+3*a);for(let k=0;k<8&&(x>32767||x< -32768);k++)x=x>32767?65534-x:-65536-x;wet=limit(x,-32768,32767);}
  if(mode===1){const target=i<64?i*anchor/64:anchor+(i-64)*(127-anchor)/63;wet=sample(target);}
  if(mode===2){const bias=p-50;wet=limit(dry*(1+(dry>=0?bias:-bias)*512/32767),-32768,32767);}
  if(mode===3){const target=i<=anchor?(i*i)/anchor:127-((127-i)*(127-i))/(127-anchor);wet=sample(target);}
  if(mode===4){const near=(source[(i+127)%128]+source[(i+1)%128])/2;wet=limit(dry+(dry-near)*(p-50)*3/50,-32768,32767);}
  /* FOLD amount controls input drive; other modes blend the transformed result. */
  out[i]=Math.round(limit(dry+(wet-dry)*a,-32768,32767));
 }
 return out;
}
