import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import vm from 'node:vm';
const root=path.resolve(import.meta.dirname,'..');
const src=fs.readFileSync(path.join(root,'WebInstaller.html'),'utf8').match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
assert.ok(src,'install script');
const env={crypto:crypto.webcrypto,Uint8Array,ArrayBuffer,TextEncoder,console,module:{exports:{}},setTimeout,clearTimeout};
env.globalThis=env;
vm.runInNewContext(src,env,{filename:'V1.48.3-safe-handoff'});
const {Engine,C,wrap}=env.module.exports;
const QUERY=Array.from(C.QUERY);
function identity(text){
 const b=new Uint8Array(34); b.set([0,0x59,0x11,27,0,0]);
 b.set(Array.from(text,x=>x.charCodeAt(0)),6);
 let sum=0;for(let i=6;i<33;i++)sum=(sum+b[i])&255;
 b[33]=(~sum)&255;return wrap(b);
}
function rig(initial,changeAfter=Infinity,replacement=initial){
 const listeners=new Map();const sent=[];const began=Date.now();
 const input={id:'in1',name:'Felucca Update',state:'connected',async open(){},async close(){},addEventListener:(k,v)=>listeners.set(k,v),removeEventListener:(k)=>listeners.delete(k)};
 const output={id:'out1',name:'Felucca Update',state:'connected',async open(){},async close(){},send(msg){
  const now=Date.now()-began;
  sent.push({at:now,bytes:[...msg]});
  if(JSON.stringify([...msg])===JSON.stringify(QUERY)){
    const current=now>=changeAfter?replacement:initial;
    setTimeout(()=>listeners.get('midimessage')?.({data:identity(current)}),0);
  }
 }};
 const logs=[];const e=new Engine({inputs:new Map([['in',input]]),outputs:new Map([['out',output]])},s=>logs.push(String(s)),()=>{});
 return {e,sent,logs,begin:began};
}
// A source app that comes back RUNNING must NOT be misrepresented as a loader.
{
 const {e,sent,logs}=rig('FM-1_900');
 const began=Date.now();
 const found=await e.waitFor(id=>id.text==='ota-FM-1_015',2600,{handoff:true,originalIdentity:'FM-1_900',quietMs:380,runningAbortMs:1100,probeIntervalMs:110,pollMs:60,allowReadOnlyFallback:true});
 assert.equal(found,null);
 assert.equal(e.lastHandoff?.state,'running-returned');
 assert.equal(e.lastHandoff?.identity,'FM-1_900');
 assert.ok(Date.now()-began>=380);
 assert.ok(sent.every(e=>e.at>=370),'no MIDI query or write during quiet interval');
 assert.ok(sent.every(e=>JSON.stringify(e.bytes)===JSON.stringify(QUERY)),'only read-only identity queries used');
 assert.ok(logs.some(s=>s.includes('OTA HANDOFF FAILED')));
}
// A late but genuine loader is still recognized using identity, not port name.
{
 const {e,sent}=rig('FM-1_900',580,'ota-FM-1_015');
 const found=await e.waitFor(id=>id.text==='ota-FM-1_015',3000,{handoff:true,originalIdentity:'FM-1_900',quietMs:380,runningAbortMs:1700,probeIntervalMs:130,pollMs:40,allowReadOnlyFallback:true});
 assert.equal(found?.id.text,'ota-FM-1_015');
 assert.equal(e.lastHandoff?.state,'loader-confirmed');
 assert.ok(sent.every(e=>e.at>=370));
 assert.ok(sent.every(e=>JSON.stringify(e.bytes)===JSON.stringify(QUERY)));
 await found.link.close();
}
assert.ok(src.includes('handoff:true, originalIdentity:initial.id.text'));
assert.ok(src.includes('No WRITE occurred'));
console.log('PASS V1.48.3: quiet USB handoff, original running app abort, genuine loader recognized, identity queries only');
