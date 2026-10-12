import {clamp} from './dsp.mjs';

export const NOTE_NAMES=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
export const SCALE_MODES=['IONIAN','DORIAN','PHRYGIAN','LYDIAN','MIXOLYDIAN','AEOLIAN','LOCRIAN','HARM MIN','MELODIC MIN'];
export const KEYLOCK_MODES=['OFF','GUIDE','LOCK'];
export const CHORD_MODES=['OFF','TRIAD','7TH'];

const SCALES={
 'IONIAN':[0,2,4,5,7,9,11],
 'DORIAN':[0,2,3,5,7,9,10],
 'PHRYGIAN':[0,1,3,5,7,8,10],
 'LYDIAN':[0,2,4,6,7,9,11],
 'MIXOLYDIAN':[0,2,4,5,7,9,10],
 'AEOLIAN':[0,2,3,5,7,8,10],
 'LOCRIAN':[0,1,3,5,6,8,10],
 'HARM MIN':[0,2,3,5,7,8,11],
 'MELODIC MIN':[0,2,3,5,7,9,11]
};
const FIFTHS=[0,7,2,9,4,11,6,1,8,3,10,5];

export function makeHarmony(){return {root:0,mode:'IONIAN',keylock:'OFF',chord:'OFF'};}
export function validateHarmony(value){
 const d=makeHarmony(),h=value&&typeof value==='object'&&!Array.isArray(value)?value:{};
 if('root'in h&&(!Number.isInteger(h.root)||h.root<0||h.root>11))throw Error('Invalid harmony root');
 if('mode'in h&&!SCALE_MODES.includes(h.mode))throw Error('Invalid harmony mode');
 if('keylock'in h&&!KEYLOCK_MODES.includes(h.keylock))throw Error('Invalid KEYLOCK mode');
 if('chord'in h&&!CHORD_MODES.includes(h.chord))throw Error('Invalid harmony chord mode');
 return {root:h.root??d.root,mode:h.mode??d.mode,keylock:h.keylock??d.keylock,chord:h.chord??d.chord};
}
export function scaleIntervals(harmony){const h=validateHarmony(harmony);return SCALES[h.mode];}
export function scalePitchClasses(harmony){const h=validateHarmony(harmony);return SCALES[h.mode].map(x=>(x+h.root)%12);}
export function pitchClassName(pc){return NOTE_NAMES[(pc%12+12)%12];}
export function midiNoteName(note){if(!Number.isInteger(note)||note<0||note>127)return '?';return `${NOTE_NAMES[note%12]}${Math.floor(note/12)-1}`;}
export function isInScale(note,harmony){return scalePitchClasses(harmony).includes(((note%12)+12)%12);}

// Nearest-note quantizer. On an exact tie ETCH chooses downward so repeated
// chromatic gestures do not drift sharp when KEYLOCK is enabled.
export function quantizeNote(note,harmony){
 if(!Number.isFinite(note))throw Error('Invalid note');
 const n=clamp(Math.round(note),0,127),pcs=new Set(scalePitchClasses(harmony)),pc=n%12;
 if(pcs.has(pc))return n;
 for(let d=1;d<=6;d++){
  const down=n-d,up=n+d;
  if(down>=0&&pcs.has((down%12+12)%12))return down;
  if(up<=127&&pcs.has(up%12))return up;
 }
 return n;
}
export function scaleDegree(note,harmony){
 const h=validateHarmony(harmony),q=quantizeNote(note,h),pc=q%12,interval=(pc-h.root+12)%12;
 return SCALES[h.mode].indexOf(interval);
}
function chordSize(mode){return mode==='7TH'?4:mode==='TRIAD'?3:1;}
export function buildDiatonicChord(note,velocity,harmony,chordMode=null,length=1){
 const h=validateHarmony(harmony),mode=chordMode??h.chord,size=chordSize(mode),root=quantizeNote(note,h),scale=SCALES[h.mode],degree=scaleDegree(root,h);
 if(size===1||degree<0)return [{note:root,velocity:clamp(Number.isFinite(velocity)?velocity:.85,0,1),length}];
 const rootInterval=scale[degree],raw=[];
 for(let i=0;i<size;i++){
  const absoluteDegree=degree+i*2,index=absoluteDegree%scale.length,oct=Math.floor(absoluteDegree/scale.length);
  raw.push(root+(scale[index]+12*oct-rootInterval));
 }
 let shift=0;while(Math.max(...raw)+shift>127)shift-=12;while(Math.min(...raw)+shift<0)shift+=12;
 const v=clamp(Number.isFinite(velocity)?velocity:.85,0,1);
 return raw.map((n,i)=>({note:n+shift,velocity:clamp(v*(1-i*.035),0,1),length}));
}
function qualityFromTriad(notes){
 if(notes.length<3)return '';
 const a=(notes[1].note-notes[0].note+120)%12,b=(notes[2].note-notes[0].note+120)%12;
 if(a===4&&b===7)return '';
 if(a===3&&b===7)return 'm';
 if(a===3&&b===6)return 'dim';
 if(a===4&&b===8)return 'aug';
 return '';
}
export function chordNameForPitchClass(pc,harmony){
 const h=validateHarmony(harmony),candidate=60+((pc-0+12)%12),root=quantizeNote(candidate,h),triad=buildDiatonicChord(root,.8,h,'TRIAD');
 return pitchClassName(root%12)+qualityFromTriad(triad);
}
export function circleOfFifthsChords(harmony){
 const h=validateHarmony(harmony),allowed=new Set(scalePitchClasses(h));
 return FIFTHS.map(x=>(h.root+x)%12).filter(pc=>allowed.has(pc)).map(pc=>({pc,name:chordNameForPitchClass(pc,h)}));
}
function dedupeNotes(notes){
 const map=new Map();for(const n of notes){const prior=map.get(n.note);if(!prior)map.set(n.note,{...n,length:n.length??1});else map.set(n.note,{note:n.note,velocity:Math.max(prior.velocity,n.velocity),length:Math.max(prior.length??1,n.length??1)});}
 return [...map.values()].sort((a,b)=>a.note-b.note).slice(0,8);
}
export function effectiveStepNotes(step,harmony){
 const h=validateHarmony(harmony),src=Array.isArray(step?.notes)?step.notes:[];
 if(!src.length)return [];
 if(h.chord!=='OFF'&&src.length===1)return buildDiatonicChord(src[0].note,src[0].velocity,h,h.chord,src[0].length??1);
 if(h.keylock==='LOCK')return dedupeNotes(src.map(n=>({note:quantizeNote(n.note,h),velocity:n.velocity,length:n.length??1})));
 return src.map(n=>({note:n.note,velocity:n.velocity,length:n.length??1}));
}
export function bakeStepHarmony(step,harmony){
 const h=validateHarmony(harmony),src=Array.isArray(step?.notes)?step.notes:[],locks={...(step?.locks||{})};
 if(!src.length)return {notes:[],locks};
 let notes;
 if(h.chord!=='OFF'){
  const root=[...src].sort((a,b)=>a.note-b.note)[0];notes=buildDiatonicChord(root.note,root.velocity,h,h.chord,root.length??1);
 }else if(h.keylock==='LOCK')notes=dedupeNotes(src.map(n=>({note:quantizeNote(n.note,h),velocity:n.velocity,length:n.length??1})));
 else notes=src.map(n=>({note:n.note,velocity:n.velocity,length:n.length??1}));
 return {notes,locks};
}
