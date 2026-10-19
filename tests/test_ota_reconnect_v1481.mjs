import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import crypto from 'node:crypto';
const R=path.resolve(import.meta.dirname,'..');
const text=fs.readFileSync(path.join(R,'WebInstaller.html'),'utf8');
const source=text.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
assert.ok(source);
const context={crypto:crypto.webcrypto,Uint8Array,ArrayBuffer,TextEncoder,console,module:{exports:{}},setTimeout,clearTimeout};
context.globalThis=context;
vm.runInNewContext(source,context,{filename:'ETCH-OTA-V1481'});
const {Engine,parseIdentity,wrap,C}=context.module.exports;
assert.equal(C.LOADER_WAIT_MS,90000);
function identity(text){
  const b=new Uint8Array(34);b.set([0,0x59,0x11,27,0,0]);
  b.set(Array.from(text, c=>c.charCodeAt(0)).slice(0,27),6);
  let sum=0;for(let i=6;i<33;i++)sum=(sum+b[i])&255;
  b[33]=(~sum)&255;return wrap(b);
}
assert.equal(parseIdentity(identity('ota-FM-1_015')).text,'ota-FM-1_015');
function factory(inpName,outName,id){
  const listeners=new Map();let outs=[];
  const inp={name:inpName,state:'connected',open:async()=>{},close:async()=>{},
    addEventListener:(name,fn)=>listeners.set(name,fn),removeEventListener:(name)=>listeners.delete(name)};
  const out={name:outName,state:'connected',open:async()=>{},close:async()=>{},
    send(p){outs.push([...p]);if(p.length===C.QUERY.length && p.every((x,i)=>x===C.QUERY[i]))
      setTimeout(()=>listeners.get('midimessage')?.({data:identity(id)}),1);}};
  return {inp,out,get sent(){return outs}};
}
const first=factory('Other MIDI Name 73','Other MIDI Name 74','ota-FM-1_015');
const logs=[];const ticks=[];
const e=new Engine({inputs:new Map([['in',first.inp]]),outputs:new Map([['out',first.out]])},m=>logs.push(m),(...a)=>ticks.push(a));
assert.equal(e.pairs(true).length,0,'unrecognized USB port names do not pass normal autopair filtering');
const found=await e.waitFor(id=>id.loader&&id.text==='ota-FM-1_015',5000,{allowReadOnlyFallback:true,phaseLabel:'Waiting for OTA loader'});
assert.equal(found?.id.text,'ota-FM-1_015');
assert(first.sent.length>=1);
assert(first.sent.every(x=>JSON.stringify(x)===JSON.stringify(C.QUERY)),'fallback sends only read-only identity queries');
assert(logs.some(x=>x.includes('Reconnected as ota-FM-1_015')));
await found.link.close();
const stranger=factory('Unknown USB MIDI 88','Unknown USB MIDI 89','ota-FM-1_999');
const l=[];
const bad=new Engine({inputs:new Map([['in',stranger.inp]]),outputs:new Map([['out',stranger.out]])},m=>l.push(m),()=>{});
const blocked=await bad.waitFor(id=>id.text==='ota-FM-1_015',1200,{allowReadOnlyFallback:true});
assert.equal(blocked,null);
assert(l.some(x=>x.includes('Different OTA loader identity')));
assert(stranger.sent.every(x=>JSON.stringify(x)===JSON.stringify(C.QUERY)));
console.log('PASS V1.48.1: loader on renamed USB ports, only identity probes, unknown loaders rejected, 90s window');
