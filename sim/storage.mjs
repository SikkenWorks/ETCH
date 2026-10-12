import {validateParams,N} from './dsp.mjs';
import {defaultWaveFx,validateWaveFx} from './wave-fx.mjs';
import {validatePerformance,makePerformance} from './performance.mjs';
export const PROJECT_MEMORY_SLOTS=16;
export const PATTERN_MEMORY_SLOTS=32;
export const MAX_BANK_BYTES=16*1024*1024;
const ASCII_NAME=/^([\x20-\x7e]){1,12}$/;
export function crc32(s){let c=0xffffffff;for(const b of new TextEncoder().encode(s)){c^=b;for(let j=0;j<8;j++)c=(c>>>1)^((c&1)?0xedb88320:0);}return ((c^0xffffffff)>>>0).toString(16).padStart(8,'0');}
export function validateWave(w){if(!w||typeof w.name!=='string'||!ASCII_NAME.test(w.name)||!Array.isArray(w.points)||w.points.length!==N||w.points.some(x=>!Number.isInteger(x)||x< -32768||x>32767))throw Error('Invalid wave');return {name:w.name,points:[...w.points],fx:validateWaveFx(w.fx??defaultWaveFx)};}
function validateName(name,label){if(typeof name!=='string'||!ASCII_NAME.test(name))throw Error(`Invalid ${label} name`);return name;}
function clonePerformance(value){return validatePerformance(value);}
export function validateSavedProject(item,waves){
 if(!item||typeof item!=='object'||Array.isArray(item))throw Error('Invalid saved project');
 const name=validateName(item.name,'project');
 if(!Array.isArray(item.slots)||item.slots.length!==3||item.slots.some(s=>!Number.isInteger(s)||s<0||s>=64||!waves[s]))throw Error('Invalid project wave references');
 const octave=Number.isInteger(item.octave)&&[12,24,36,48,60,72,84,96].includes(item.octave)?item.octave:48;
 return {name,slots:[...item.slots],params:validateParams(item.params),performance:clonePerformance(item.performance),octave};
}
export function validatePattern(item){
 if(!item||typeof item!=='object'||Array.isArray(item))throw Error('Invalid pattern');
 const name=validateName(item.name,'pattern'),base=makePerformance(),validated=clonePerformance({arp:base.arp,harmony:item.harmony,seq:item.seq});
 return {name,harmony:validated.harmony,seq:validated.seq};
}
export function validateProject(d){
 if(!d||!Array.from({length:11},(_,i)=>i+1).includes(d.version)||!Array.isArray(d.waves)||d.waves.length!==64)throw Error('Unsupported ETCH bank');
 const waves=d.waves.map(w=>w===null?null:validateWave(w));
 if(!Array.isArray(d.patches)||d.patches.length!==32)throw Error('Expected 32 patch slots');
 const patches=d.patches.map(p=>{if(p===null)return null;if(!p||typeof p.name!=='string'||!ASCII_NAME.test(p.name)||!Array.isArray(p.slots)||p.slots.length!==3||p.slots.some(s=>!Number.isInteger(s)||s<0||s>=64||!waves[s]))throw Error('Invalid patch references');return {name:p.name,slots:[...p.slots],params:validateParams(p.params),performance:validatePerformance(p.performance)};});
 let projects=Array(PROJECT_MEMORY_SLOTS).fill(null),patterns=Array(PATTERN_MEMORY_SLOTS).fill(null);
 if(d.version>=10){
  if(!Array.isArray(d.projects)||d.projects.length!==PROJECT_MEMORY_SLOTS)throw Error(`Expected ${PROJECT_MEMORY_SLOTS} project slots`);
  if(!Array.isArray(d.patterns)||d.patterns.length!==PATTERN_MEMORY_SLOTS)throw Error(`Expected ${PATTERN_MEMORY_SLOTS} pattern slots`);
  projects=d.projects.map(x=>x===null?null:validateSavedProject(x,waves));patterns=d.patterns.map(x=>x===null?null:validatePattern(x));
 }
 return {version:11,waves,patches,projects,patterns};
}
export function encode(data){const payload=JSON.stringify(validateProject(data));return JSON.stringify({format:'ETCH-HOST',version:11,payload,crc32:crc32(payload)},null,2);}
export function decode(text){if(text.length>MAX_BANK_BYTES)throw Error('File too large');const r=JSON.parse(text);if(r.format!=='ETCH-HOST'||!Array.from({length:11},(_,i)=>i+1).includes(r.version)||typeof r.payload!=='string'||crc32(r.payload)!==r.crc32)throw Error('Checksum or format mismatch');return validateProject(JSON.parse(r.payload));}
// Separate v11 namespace preserves all earlier snapshots and migrates only after a verified save.
// V0.18 added a write-ahead PENDING record; V0.20 adds per-wave FX metadata without changing the commit protocol.
// PENDING is never booted as committed data: it exists only
// to prove the complete encoded bank can be written/read before CURRENT is replaced.
export const STORAGE_KEYS=Object.freeze({current:'etch.v11.current',previous:'etch.v11.previous',pending:'etch.v11.pending'});
function validRecord(text){if(!text)return false;try{decode(text);return true;}catch{return false;}}
function discard(storage,key){try{if(typeof storage.removeItem==='function')storage.removeItem(key);else storage.setItem(key,'');return true;}catch{return false;}}
export function inspectStorage(storage){
 const current=storage.getItem(STORAGE_KEYS.current),previous=storage.getItem(STORAGE_KEYS.previous),pending=storage.getItem(STORAGE_KEYS.pending);
 return {currentPresent:!!current,currentValid:validRecord(current),previousPresent:!!previous,previousValid:validRecord(previous),pendingPresent:!!pending,pendingValid:validRecord(pending)};
}
export function save(storage,data){
 const value=encode(data),old=storage.getItem(STORAGE_KEYS.current);
 storage.setItem(STORAGE_KEYS.pending,value);
 const pending=storage.getItem(STORAGE_KEYS.pending);
 if(pending!==value||!validRecord(pending))throw Error('Pending save verification failed');
 if(validRecord(old))storage.setItem(STORAGE_KEYS.previous,old);
 storage.setItem(STORAGE_KEYS.current,pending);
 const committed=storage.getItem(STORAGE_KEYS.current);
 if(committed!==value||!validRecord(committed))throw Error('Save verification failed');
 const pendingCleared=discard(storage,STORAGE_KEYS.pending);
 return {verified:true,pendingCleared};
}
export function load(storage){
 for(const key of [STORAGE_KEYS.current,STORAGE_KEYS.previous,'etch.v10.current','etch.v10.previous','etch.v9.current','etch.v9.previous','etch.v8.current','etch.v8.previous','etch.v7.current','etch.v7.previous','etch.v6.current','etch.v6.previous','etch.v5.current','etch.v5.previous','etch.v4.current','etch.v4.previous','etch.v3.current','etch.v3.previous','etch.v2.current','etch.v2.previous','etch.current','etch.previous']){
  const text=storage.getItem(key);if(text){try{return {data:decode(text),recovered:key.endsWith('previous'),migrated:!key.startsWith('etch.v11.'),orphanPending:!!storage.getItem(STORAGE_KEYS.pending)};}catch{}}
 }return null;
}
