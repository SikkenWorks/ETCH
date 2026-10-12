import {clamp} from './dsp.mjs';

export const WAVE_NAME_CHARS=[' ',...Array.from('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.#')];
export const WAVE_NAME_MAX=12;

function supportedChar(ch){
 const upper=String(ch??' ').toUpperCase();
 return WAVE_NAME_CHARS.includes(upper)?upper:'_';
}

export function makeWaveNameEditor(name='WAVE',slot=0){
 const raw=String(name??'WAVE').slice(0,WAVE_NAME_MAX);
 const chars=Array.from({length:WAVE_NAME_MAX},(_,i)=>supportedChar(raw[i]??' '));
 const visible=raw.replace(/\s+$/,'').length;
 return {chars,cursor:clamp(Math.max(0,visible-1),0,WAVE_NAME_MAX-1),length:clamp(Math.max(1,visible),1,WAVE_NAME_MAX),slot:clamp(Math.trunc(slot)||0,0,63)};
}

export function setWaveNamePosition(state,position){
 state.cursor=clamp(Math.trunc(position)||0,0,WAVE_NAME_MAX-1);
 if(state.cursor>=state.length)state.length=state.cursor+1;
 return state;
}

export function setWaveNameLength(state,length){
 state.length=clamp(Math.trunc(length)||1,1,WAVE_NAME_MAX);
 if(state.cursor>=state.length)state.cursor=state.length-1;
 return state;
}

export function setWaveNameSlot(state,slot){state.slot=clamp(Math.trunc(slot)||0,0,63);return state;}

export function setWaveNameChar(state,ch){
 state.chars[state.cursor]=supportedChar(ch);
 if(state.cursor>=state.length)state.length=state.cursor+1;
 return state;
}

export function turnWaveNameChar(state,dir){
 const current=supportedChar(state.chars[state.cursor]);
 let i=WAVE_NAME_CHARS.indexOf(current);if(i<0)i=0;
 i=(i+(dir<0?-1:1)+WAVE_NAME_CHARS.length)%WAVE_NAME_CHARS.length;
 return setWaveNameChar(state,WAVE_NAME_CHARS[i]);
}

export function moveWaveNameCursor(state,dir){return setWaveNamePosition(state,state.cursor+(dir<0?-1:1));}

export function waveNameValue(state){
 const raw=state.chars.slice(0,state.length).join('').replace(/\s+$/,'').replace(/^\s+/,'');
 return raw||'WAVE';
}

export function displayWaveNameChar(ch){return ch===' '?'<SPACE>':ch;}
