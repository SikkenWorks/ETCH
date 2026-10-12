// Actual ETCH app controller in a minimal DOM/audio harness; this validates control routing, not CSS layout.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {compileWave} from '../sim/dsp.mjs';

const all=[],ids=new Map(),listeners=new Map(),messages=[],midiSends=[];
class Element{
 constructor(tag='div',attrs={}){this.tagName=tag.toUpperCase();this.children=[];this.dataset={};this._value=null;this.open=false;this.textContent='';this.hidden=false;this.disabled=false;this.innerHTML='';this.classList={_set:new Set(),toggle(name,force){const on=force===undefined?!this._set.has(name):Boolean(force);if(on)this._set.add(name);else this._set.delete(name);return on;},contains(name){return this._set.has(name);}};for(const [k,v]of Object.entries(attrs)){if(k==='id')ids.set(v,this);if(k==='value')this._value=v;if(k.startsWith('data-')){const key=k.slice(5).replace(/-([a-z])/g,(_,c)=>c.toUpperCase());this.dataset[key]=v;}}all.push(this);}
 get value(){return this._value??(this.tagName==='SELECT'?this.children[0]?.value??'':'');}
 set value(v){this._value=String(v);}
 get options(){return this.children;}
 append(...xs){this.children.push(...xs);} add(x){this.children.push(x);} replaceChildren(...xs){this.children=xs;this._value=null;}
 get firstChild(){return this.children[0];} focus(){} setAttribute(k,v){this[k]=v;} showModal(){this.open=true;} close(){this.open=false;} click(){return this.onclick?.();} setPointerCapture(){}
 getBoundingClientRect(){return {left:0,top:0,width:240,height:240};}
 getContext(){return new Proxy({},{get:(t,k)=>t[k]??(()=>{}),set:(t,k,v)=>(t[k]=v,true)});}
}
const html=fs.readFileSync(new URL('../sim/index.html',import.meta.url),'utf8');
const attrs=text=>Object.fromEntries([...text.matchAll(/([\w-]+)="([^"]*)"/g)].map(m=>[m[1],m[2]]));
for(const m of html.matchAll(/<(button|input|select|canvas|div|dialog|span|nav|p|h2)\b([^>]*)>/g)){
 const a=attrs(m[2]),el=new Element(m[1],a);
 if(m[1]==='select'){
  const end=html.indexOf('</select>',m.index),inner=html.slice(m.index+m[0].length,end);
  for(const option of inner.matchAll(/<option([^>]*)>([^<]*)<\/option>/g)){const oa=attrs(option[1]),o=new Element('option');o.textContent=option[2];o.value=oa.value??option[2];el.add(o);if(/\bselected\b/.test(option[1]))el.value=o.value;}
 }
}
globalThis.Option=class extends Element{constructor(text,value){super('option');this.textContent=String(text);this.value=value;}};
if(ids.get('pageTitle'))ids.get('pageTitle').textContent='PLAY';if(ids.get('state'))ids.get('state').textContent='AUDIO OFF';if(ids.get('transportState'))ids.get('transportState').textContent='SEQ TRANSPORT IDLE';
function hasData(el,key){return key in el.dataset;}
globalThis.document={
 getElementById:id=>ids.get(id),createElement:tag=>new Element(tag),
 querySelectorAll:selector=>{
  const m=selector.match(/^\[data-([\w-]+)\]$/);if(!m)return [];const key=m[1].replace(/-([a-z])/g,(_,c)=>c.toUpperCase());return all.filter(el=>hasData(el,key));
 },
 addEventListener:(k,f)=>listeners.set(k,f)
};
globalThis.window={addEventListener:(k,f)=>listeners.set(k,f)};
const storage=new Map();globalThis.localStorage={getItem:k=>storage.get(k)??null,setItem:(k,v)=>storage.set(k,v)};
globalThis.Worker=class{postMessage(data){if(data.waves)queueMicrotask(()=>this.onmessage?.({data:{id:data.id,bank:data.waves.map(w=>compileWave(w.points,w.fx))}}));}terminate(){}};
globalThis.AudioContext=class{
 constructor(){this.audioWorklet={addModule:async()=>{}};this.destination={};}
 createAnalyser(){return {fftSize:1024,getFloatTimeDomainData:a=>a.fill(0),connect(){return this;}};}
 createGain(){return {gain:{value:1},connect(){return this;}};}
 async resume(){} async close(){}
};
let workletNode=null;globalThis.AudioWorkletNode=class{constructor(){workletNode=this;this.port={postMessage:m=>messages.push(m)};}connect(){return this;}};
const midiDevice={id:'m1',name:'TEST MIDI',onmidimessage:null};
const midiOutputDevice={id:'o1',name:'TEST OUT',send:data=>midiSends.push(Uint8Array.from(data))};
Object.defineProperty(globalThis,'navigator',{value:{requestMIDIAccess:async()=>({inputs:new Map([[midiDevice.id,midiDevice]]),outputs:new Map([[midiOutputDevice.id,midiOutputDevice]]),onstatechange:null})},configurable:true});
const realTimeout=globalThis.setTimeout;globalThis.setTimeout=()=>1;globalThis.clearTimeout=()=>{};globalThis.requestAnimationFrame=()=>{};

try{
 await import('../sim/app.mjs');await Promise.resolve();
 const $=id=>ids.get(id);
 const hw=name=>all.find(b=>b.dataset.hwPage===name);
 const encoder=(name,turn)=>all.find(b=>b.dataset.encoder===name&&+b.dataset.turn===turn);
 const tool=name=>all.find(b=>b.dataset.tool===name);
 const answer=async text=>{const b=$('dialogActions').children.find(b=>b.textContent===text);assert(b,`Missing dialog action ${text}`);await b.onclick();await Promise.resolve();};

 assert.equal($('slotA').children.length,4);assert.equal($('patchSlot').children.length,32);assert.equal($('projectSlot').children.length,16);assert.equal($('patternSlot').children.length,32);assert.equal($('controls').children.length,4);assert.equal($('midiChannel').children.length,17);assert.equal($('transferSlot').children.length,64);
 assert.equal(all.filter(el=>hasData(el,'note')).length,27,'FM-1 keybed must expose 27 playable keys');
 assert.deepEqual(['FX','ENV','MOD WAVE','DRAW','SYSTEM','PLAY','ARP','SEQ'].map(name=>Boolean(hw(name))),Array(8).fill(true));
 assert($('selButton')&&$('hardwareSave')&&$('transport')&&$('record'),'FM-1 SEL/SAVE/PLAY-STOP/REC buttons must exist');
 assert(encoder('select',-1)&&encoder('select',1)&&encoder('presets',-1)&&encoder('algorithm',1),'FM-1 left rotary controls must be represented');

 await $('power').onclick();assert.equal($('state').textContent,'8 VOICES · 4×');assert(messages.some(m=>m.type==='bank'));
 $('masterLevel').value='.42';$('masterLevel').oninput();assert.equal($('masterLevel').value,'.42');
 $('batteryLevel').value='37';$('batteryLevel').oninput();assert.equal($('batteryValue').textContent,'37%');$('batteryCharging').checked=true;$('batteryCharging').onchange();assert.equal($('batteryValue').textContent,'37% · CHARGING');

 await $('enableMidi').onclick();assert.equal($('midiInput').value,'m1');assert.equal($('midiOutput').value,'o1');assert.equal($('sendSyx').disabled,false);
 $('transferKind').value='wave';$('transferKind').onchange();$('transferSlot').value='0';$('sendSyx').onclick();assert.equal(midiSends.length,1);assert.equal(midiSends[0][0],0xf0);assert.equal(midiSends[0].at(-1),0xf7);
 midiDevice.onmidimessage({data:[0x90,64,100]});assert(messages.some(m=>m.type==='on'&&m.note===64&&m.velocity>0));midiDevice.onmidimessage({data:[0xe0,0,96]});assert(messages.some(m=>m.type==='bend'&&m.value>0));midiDevice.onmidimessage({data:[0xb0,1,127]});assert(messages.some(m=>m.type==='params'&&m.params.morph===1));midiDevice.onmidimessage({data:[0x80,64,0]});assert(messages.some(m=>m.type==='off'&&m.note===64));
 $('midiChannel').value='1';$('midiChannel').onchange();const filtered=messages.filter(m=>m.type==='on').length;midiDevice.onmidimessage({data:[0x90,66,100]});assert.equal(messages.filter(m=>m.type==='on').length,filtered);$('midiChannel').value='-1';
 $('midiClockMode').value='MIDI';$('midiClockMode').onchange();for(let i=0;i<30;i++)midiDevice.onmidimessage({data:[0xf8],timeStamp:i*(60000/120/24)});assert(messages.some(m=>m.type==='clock'&&m.bpm>119.9&&m.bpm<120.1));

 // SELECT follows the ETCH HOME-page ring: PLAY -> MORPH -> FILTER -> LIBRARY -> PLAY.
 assert.equal($('pageTitle').textContent,'PLAY');encoder('select',1).onclick();await Promise.resolve();assert.equal($('pageTitle').textContent,'MORPH');assert.equal($('controls').children.length,4);
 encoder('select',1).onclick();await Promise.resolve();assert.equal($('pageTitle').textContent,'FILTER');assert.equal($('controls').children.length,4);
 encoder('select',1).onclick();await Promise.resolve();assert.equal($('pageTitle').textContent,'LIBRARY');encoder('select',1).onclick();await Promise.resolve();assert.equal($('pageTitle').textContent,'PLAY');

 await hw('ENV').onclick();assert.equal($('pageTitle').textContent,'ENV · AMP');assert.equal($('controls').children.length,4);encoder('select',1).onclick();assert.equal($('pageTitle').textContent,'ENV · FILTER');assert.equal($('controls').children.length,4);
 const filterAttack=$('controls').children[0].children[1];filterAttack.value='.2';filterAttack.oninput();assert(messages.at(-1).params.fAttack===.2);
 await hw('MOD WAVE').onclick();assert.equal($('pageTitle').textContent,'MOD WAVE · CORE');encoder('select',1).onclick();assert.equal($('pageTitle').textContent,'MOD WAVE · RATE');
 await hw('SYSTEM').onclick();assert.equal($('controls').children.length,4);
 await hw('FX').onclick();assert.equal($('controls').children.length,4);assert.equal($('controls').children[0].children[0].textContent,'DRIVE');assert.equal($('controls').children[1].children[0].textContent,'CHORUS');assert.equal($('controls').children[2].children[0].textContent,'DELAY');assert.equal($('controls').children[3].children[0].textContent,'REVERB');const rev=$('controls').children[3].children[1];rev.value='.44';rev.oninput();assert.equal(messages.at(-1).params.reverb,.44);
 await hw('PLAY').onclick();assert.equal($('controls').children[3].children[0].textContent,'SPACE');const space=$('controls').children[3].children[1];space.value='.31';space.oninput();assert.equal(messages.at(-1).params.reverb,.31);

 $('editSource').value='C';await $('editWave').onclick();assert.equal($('pageTitle').textContent,'DRAW');assert.equal($('editTarget').value,'C');await hw('FX').onclick();assert.equal($('pageTitle').textContent,'WAVE FX · I');assert.deepEqual($('controls').children.map(x=>x.children[0].textContent),['DRIVE','BITCRUSH','CHORUS','DELAY']);const waveDrive=$('controls').children[0].children[1];waveDrive.value='.73';waveDrive.oninput();assert.equal(waveDrive.value,'.73');await hw('FX').onclick();assert.equal($('pageTitle').textContent,'WAVE FX · II');assert.deepEqual($('controls').children.map(x=>x.children[0].textContent),['REVERB','FOLD','SMOOTH','FX MIX']);await hw('DRAW').onclick();assert.equal($('pageTitle').textContent,'DRAW');$('selButton').onclick();assert.equal($('drawToolMenu').hidden,false);
 tool('invert').onclick();const leave=hw('PLAY').onclick();await Promise.resolve();assert($('dialog').open);await answer('CANCEL');await leave;assert.equal($('pageTitle').textContent,'DRAW');
 $('saveSlot').value=10;$('waveName').value='UI TEST';$('patchSlot').value=7;const leaveAndSave=hw('PLAY').onclick();await Promise.resolve();await answer('SAVE WAVE');await leaveAndSave;assert.equal($('pageTitle').textContent,'PLAY');assert.equal($('slotC').value,'10');assert.equal($('patchSlot').value,'7');assert(storage.has('etch.v11.current'));
 $('patchName').value='UI PATCH';await $('savePatch').onclick();assert($('patchSlot').children[7].textContent.includes('UI PATCH'));
 $('projectName').value='UI PROJECT';$('projectSlot').value='2';await $('saveProject').onclick();assert($('projectSlot').children[2].textContent.includes('UI PROJECT'));
 $('patternName').value='UI PATTERN';$('patternSlot').value='3';await $('savePattern').onclick();assert($('patternSlot').children[3].textContent.includes('UI PATTERN'));
 // PROJECT load restores the saved working sound after a later edit.
 const changedSpace=$('controls').children[3].children[1];changedSpace.value='.61';changedSpace.oninput();const loadProjectPromise=$('loadProject').onclick();await Promise.resolve();assert($('dialog').open);await answer('LOAD PROJECT');await loadProjectPromise;assert(Math.abs(messages.filter(m=>m.type==='params').at(-1).params.reverb-.31)<1e-6);
 // PATTERN load restores sequence/harmony only. Change sequence length, then reload the saved 16-step pattern.
 await hw('SEQ').onclick();let seqLenControl=$('controls').children[0].children[1];seqLenControl.value='8';seqLenControl.onchange();const loadPatternPromise=$('loadPattern').onclick();await Promise.resolve();assert($('dialog').open);await answer('LOAD PATTERN');await loadPatternPromise;assert.equal(messages.filter(m=>m.type==='performance').at(-1).performance.seq.length,16);await hw('PLAY').onclick();

 // Hardware wave workflow: LIBRARY focus + EDIT must open the focused A/B/C source without using the desktop-only editor picker.
 encoder('select',-1).onclick();await Promise.resolve();assert.equal($('pageTitle').textContent,'LIBRARY');encoder('algorithm',-1).onclick();assert.equal($('editSource').value,'B');await hw('DRAW').onclick();assert.equal($('pageTitle').textContent,'DRAW');assert.equal($('editTarget').value,'B');
 // Hardware SAVE enters an on-device name/slot editor. K1=slot, K2=position, K3=character, K4=length; SAVE commits.
 $('saveSlot').value='12';$('waveName').value='TRIANGLE';await $('hardwareSave').onclick();assert.equal($('pageTitle').textContent,'SAVE WAVE');assert.deepEqual($('controls').children.map(x=>x.children[0].textContent),['SLOT','POSITION','CHAR','LENGTH']);const hwSaveControls=$('controls').children.map(x=>x.children[1]);hwSaveControls[0].value='12';hwSaveControls[0].onchange();hwSaveControls[1].value='0';hwSaveControls[1].onchange();hwSaveControls[2].value='Z';hwSaveControls[2].onchange();assert($('waveName').value.startsWith('Z'));await $('hardwareSave').onclick();assert.equal($('pageTitle').textContent,'DRAW');assert.equal($('slotB').value,'12');await hw('PLAY').onclick();assert.equal($('pageTitle').textContent,'PLAY');

 // OCT-/+ operate the real FM-1 octave function and keybed remains shared-note safe.
 const octaveBefore=+$('octave').value;$('octUp').onclick();assert(+$('octave').value>octaveBefore);$('octDown').onclick();assert.equal(+$('octave').value,octaveBefore);
 const before=messages.filter(m=>m.type==='on').length,button=all.find(b=>b.dataset.note===0);button.onpointerdown({pointerId:1});listeners.get('keydown')({key:'a',code:'KeyA',target:{tagName:'BODY'},repeat:false});assert.equal(messages.filter(m=>m.type==='on').length-before,1);button.onpointerup({pointerId:1});const offBefore=messages.filter(m=>m.type==='off').length;listeners.get('keyup')({code:'KeyA'});assert.equal(messages.filter(m=>m.type==='off').length-offBefore,1);

 await $('editWave').onclick();tool('flatten').onclick();const discard=hw('PLAY').onclick();await Promise.resolve();await answer('DISCARD');await discard;assert.equal($('pageTitle').textContent,'PLAY');
 await hw('ARP').onclick();assert.equal($('controls').children[0].children[0].textContent,'MODE');const arpMode=$('controls').children[0].children[1];arpMode.value='UP';arpMode.onchange();const heldBefore=messages.filter(m=>m.type==='heldOn').length;button.onpointerdown({pointerId:2});assert.equal(messages.filter(m=>m.type==='heldOn').length-heldBefore,1);button.onpointerup({pointerId:2});assert(messages.some(m=>m.type==='heldOff'));
 await hw('SEQ').onclick();assert.deepEqual($('controls').children.map(x=>x.children[0].textContent),['LEN','RATE','SWING','GATE']);const lenControl=$('controls').children[0].children[1];assert.deepEqual(lenControl.children.map(o=>+o.value),[8,16,32,64,128,256]);lenControl.value='256';lenControl.onchange();assert.equal(messages.at(-1).performance.seq.length,256);lenControl.value='8';lenControl.onchange();assert.equal(messages.at(-1).performance.seq.length,8);lenControl.value='16';lenControl.onchange();$('selButton').onclick();assert.equal($('pageTitle').textContent,'SEQ · STEP');$('selButton').onclick();assert.equal($('pageTitle').textContent,'SEQ');
 // Physical SAVE on SEQ defaults to PATTERN and exposes SAVE / SAVE AS / LOAD / NEW.
 await $('hardwareSave').onclick();assert.equal($('pageTitle').textContent,'SAVE MENU');assert.equal($('controls').children[0].children[1].value,'PATTERN');let actionControl=$('controls').children[1].children[1];assert.deepEqual(actionControl.children.map(o=>o.value),['SAVE','SAVE AS','LOAD','NEW']);assert.equal(actionControl.value,'SAVE');await $('hardwareSave').onclick();
 // SAVE AS exposes SLOT + NAME LEN, then makes the new destination the active SAVE slot.
 await $('hardwareSave').onclick();actionControl=$('controls').children[1].children[1];actionControl.value='SAVE AS';actionControl.onchange();assert.deepEqual($('controls').children.map(x=>x.children[0].textContent),['OBJECT','ACTION','SLOT','NAME LEN']);const saveAsControls=$('controls').children.map(x=>x.children[1]);saveAsControls[2].value='4';saveAsControls[2].onchange();await $('hardwareSave').onclick();assert($('patternSlot').children[4].textContent.includes('UI PATTERN'));await $('hardwareSave').onclick();assert.equal($('controls').children[1].children[1].value,'SAVE');await $('hardwareSave').onclick();
 // NEW clears the pattern without touching the sound.
 await $('hardwareSave').onclick();actionControl=$('controls').children[1].children[1];actionControl.value='NEW';actionControl.onchange();const newPromise=$('hardwareSave').onclick();await Promise.resolve();if($('dialog').open)await answer('NEW PATTERN');await newPromise;assert(messages.filter(m=>m.type==='performance').at(-1).performance.seq.steps.every(step=>step.notes.length===0));
 // V0.24: SELECT is the stopped step cursor and held-note length encoder, while HOLD and TRIGGER remain separate layers.
 encoder('select',1).onclick();await Promise.resolve();assert.equal($('pageTitle').textContent,'SEQ');assert($('status').textContent.includes('02'));encoder('algorithm',1).onclick();assert($('status').textContent.includes('step selection is on SELECT'));
 // Direct step entry works while stopped, with no REC/playback requirement.
 const directBefore=messages.filter(m=>m.type==='performance').length;button.onpointerdown({pointerId:30});button.onpointerup({pointerId:30});assert(messages.filter(m=>m.type==='performance').length>directBefore);let directPerf=messages.filter(m=>m.type==='performance').at(-1).performance;assert.deepEqual(directPerf.seq.steps[1].notes.map(n=>n.note),[48]);assert(button.classList.contains('seq-trigger'),'selected stopped-step trigger should use the TRIGGER layer highlight');
 encoder('select',-1).onclick();assert($('status').textContent.includes('01'));
 // Hold entered note(s) while turning SELECT to stretch their length across following steps.
 button.onpointerdown({pointerId:31});encoder('select',1).onclick();encoder('select',1).onclick();let lengthPerf=messages.filter(m=>m.type==='performance').at(-1).performance;assert.equal(lengthPerf.seq.steps[0].notes.find(n=>n.note===48).length,3);assert($('status').textContent.includes('NOTE LENGTH 3 STEPS'));button.onpointerup({pointerId:31});encoder('select',-1).onclick();assert($('status').textContent.includes('02'));const overlayButton=all.find(b=>b.dataset.note===7);overlayButton.onpointerdown({pointerId:32});overlayButton.onpointerup({pointerId:32});let layeredPerf=messages.filter(m=>m.type==='performance').at(-1).performance;assert.equal(layeredPerf.seq.steps[0].notes.find(n=>n.note===48).length,3);assert.deepEqual(layeredPerf.seq.steps[1].notes.map(n=>n.note),[48,55]);assert(button.classList.contains('seq-hold')&&button.classList.contains('seq-trigger'),'same pitch may exist as both incoming HOLD and a normal TRIGGER on this step');assert(overlayButton.classList.contains('seq-trigger'),'new normal note should be visible as TRIGGER layer over HOLD');encoder('select',-1).onclick();assert($('status').textContent.includes('01'));
 // Pressing the physical SEQ button again exposes HARMONY; SELECT remains reserved for steps.
 await hw('SEQ').onclick();assert.equal($('pageTitle').textContent,'SEQ · HARMONY');assert.deepEqual($('controls').children.map(x=>x.children[0].textContent),['ROOT','MODE','KEYLOCK','CHORD']);
 const harmonyControls=$('controls').children.map(x=>x.children[1]);harmonyControls[0].value='0';harmonyControls[0].onchange();harmonyControls[1].value='IONIAN';harmonyControls[1].onchange();harmonyControls[2].value='LOCK';harmonyControls[2].onchange();harmonyControls[3].value='TRIAD';harmonyControls[3].onchange();assert.equal(messages.at(-1).performance.harmony.keylock,'LOCK');assert.equal(messages.at(-1).performance.harmony.chord,'TRIAD');
 await hw('SEQ').onclick();assert.equal($('pageTitle').textContent,'SEQ');
 $('transport').onclick();assert.equal($('transportState').textContent,'SEQ PLAY · 01');assert(messages.some(m=>m.type==='transport'&&m.action==='start'));workletNode.port.onmessage({data:{type:'seq-step',seqStep:0}});workletNode.port.onmessage({data:{type:'seq-led',notes:[48,52,55]}});assert(button.classList.contains('down'));assert(all.find(b=>b.dataset.note===4).classList.contains('down'),'sequencer playback should drive the FM-1 key LEDs');
 // Changing OCT-/+ must only transpose the live keybed. It must not send PANIC/STOP or reset the running sequence.
 const panicBeforeOct=messages.filter(m=>m.type==='panic').length,stopBeforeOct=messages.filter(m=>m.type==='transport'&&m.action==='stop').length,octRunBefore=+$('octave').value;$('octUp').onclick();assert(+$('octave').value>octRunBefore);assert($('transportState').textContent.startsWith('SEQ PLAY'));assert.equal(messages.filter(m=>m.type==='panic').length,panicBeforeOct);assert.equal(messages.filter(m=>m.type==='transport'&&m.action==='stop').length,stopBeforeOct);$('octDown').onclick();assert.equal(+$('octave').value,octRunBefore);assert($('transportState').textContent.startsWith('SEQ PLAY'));
 $('record').onclick();assert($('transportState').textContent.startsWith('SEQ RECORDING')||$('transportState').textContent.startsWith('SEQ REC'));const chord2=all.find(b=>b.dataset.note===4);button.onpointerdown({pointerId:3});chord2.onpointerdown({pointerId:4});assert(messages.some(m=>m.type==='performance'&&m.performance.seq.steps.some(step=>step.notes?.length===2)));button.onpointerup({pointerId:3});chord2.onpointerup({pointerId:4});$('transport').onclick();assert($('transportState').textContent.startsWith('SEQ STOP'));
 // KEYLOCK + TRIAD can be baked destructively into the selected step with SEL.
 await hw('SEQ').onclick();assert.equal($('pageTitle').textContent,'SEQ · HARMONY');const bake=$('selButton').onclick();await Promise.resolve();assert($('dialog').open);await answer('BAKE');await bake;const baked=messages.filter(m=>m.type==='performance').at(-1).performance.seq.steps[0].notes;assert.deepEqual(baked.map(n=>n.note),[48,52,55]);
 console.log('UI controller: enlarged readable 240x240 hardware workflow, battery/charging telemetry, focused wave EDIT + rotary wave naming, octave-safe sequencer playback, SELECT step cursor + held-note length, direct entry, key LEDs, harmony, ARP/SEQ, FX, ENV and MIDI/SysEx passed');
}finally{globalThis.setTimeout=realTimeout;}
