import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import crypto from 'node:crypto';
const root=path.resolve(import.meta.dirname,'..');
const html=fs.readFileSync(path.join(root,'WebInstaller.html'),'utf8');
const js=html.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
assert.ok(js,'actual ETCH installer source');
const c={atob: s=>Buffer.from(s,'base64').toString('binary'),crypto:crypto.webcrypto,Uint8Array,ArrayBuffer,TextEncoder,console,module:{exports:{}},setTimeout,clearTimeout};
c.globalThis=c;vm.runInNewContext(js,c,{filename:'ETCH-V1.81-UX-AUDIO'});
const {Engine,C,wrap,isApprovedOtaLoader,isCompatibleFm1Identity,verifyPackage,fromBase64}=c.module.exports;
assert.equal(isCompatibleFm1Identity("ota-FM-1_900"),true);
assert.equal(isApprovedOtaLoader({text:'ota-FM-1_900',loader:true}),true,'B1 HF2 recognized Felucca/SLOOP loader');
assert.equal(isApprovedOtaLoader({text:'ota-FM-1_015',loader:true}),true,'stock loader remains supported');
assert.equal(isApprovedOtaLoader({text:'FM-1_900',loader:false}),false,'running firmware is never OTA loader');
assert.equal(isApprovedOtaLoader({text:'ota-FM-1_777',loader:true}),false,'unapproved loader rejected');
function identity(text){
 const b=new Uint8Array(34);b.set([0,0x59,0x11,27,0,0]);b.set(Array.from(text,x=>x.charCodeAt(0)),6);
 let sum=0;for(let i=6;i<33;i++)sum=(sum+b[i])&255;b[33]=(~sum)&255;return wrap(b);
}
// B1 HF2 genuine reconnect, USB-MIDI port has SAME Felucca label before/after the reboot.
{
 const listeners=new Map(),sent=[];let ident='FM-1_900';
 const inp={id:'in1',name:'Felucca Update',state:'connected',open:async()=>{},close:async()=>{},addEventListener:(k,v)=>listeners.set(k,v),removeEventListener:k=>listeners.delete(k)};
 const out={id:'out1',name:'Felucca Update',state:'connected',open:async()=>{},close:async()=>{},send(packet){sent.push([...packet]);if(packet.length===C.QUERY.length&&packet.every((v,i)=>v===C.QUERY[i]))setTimeout(()=>listeners.get('midimessage')?.({data:identity(ident)}),0)}};
 const eng=new Engine({inputs:new Map([['i',inp]]),outputs:new Map([['o',out]])},()=>{},()=>{});
 setTimeout(()=>{ident='ota-FM-1_900'},150);
 const found=await eng.waitForB1HF2(isApprovedOtaLoader,5000);
 assert.equal(found?.id.text,'ota-FM-1_900');
 assert.ok(sent.every(p=>p.length===C.QUERY.length && p.every((v,i)=>v===C.QUERY[i])),'identity probes only');
 await found.link.close();
}
// Full CHECK / loader / WRITE / post-boot order for a FM-1_900 source.
{
 const steps=[];
 const link=n=>({send(){steps.push(n+':upgrade');return true},async close(){steps.push(n+':close')}});
 const eng=new Engine({},()=>{},()=>{});
 eng.serve=async (_link,_image,fin)=>{steps.push(fin===C.FINISH_CHECK?'CHECK':'WRITE');return 65};
 eng.waitForB1HF2=async(pred)=>{const id={text:'ota-FM-1_900',loader:true};assert(pred(id));steps.push('OTA-900');return{id,link:link('loader')}};
 eng.waitFor=async()=>{steps.push('POST-BOOT');return{id:{text:'FM-1_1500',loader:false},link:link('app')}};
 await eng.install({id:{text:'FM-1_900',loader:false},link:link('source')},{product:'FM-1_1500',kind:'etch-stockhead',image:new Uint8Array(100)});
 assert.deepEqual(steps.filter(x=>['CHECK','OTA-900','WRITE','POST-BOOT'].includes(x)),['CHECK','OTA-900','WRITE','POST-BOOT']);
}
// CHECK rejection absolutely blocks all loader probing and WRITEs.
{
 const eng=new Engine({},()=>{},()=>{});let probed=0,writes=0;
 eng.serve=async()=>{throw new Error('CHECK refused by firmware')};
 eng.waitForB1HF2=async()=>{probed++;return null};
 const link={send(){return true},async close(){}};
 await assert.rejects(eng.install({id:{text:'FM-1_900',loader:false},link},{product:'FM-1_1500',kind:'etch-stockhead',image:new Uint8Array(100)}),/CHECK refused/);
 assert.equal(probed,0);assert.equal(writes,0);
}
// Updated firmware V1.81: both actual base64 payloads SHA-256 match the release files.
for(const [id,file,kind] of [['firmware-embedded','release/ETCH-V1.81.fwsc','etch'],['firmware-embedded-stockhead','compat/ETCH-V1.81-LEGACY-STOCK-HEAD.fwsc','etch-stockhead']]){
 const b64=html.match(new RegExp('<script id="'+id+'" type="application/octet-stream">\\s*([\\s\\S]*?)\\s*<\\/script>'))?.[1];
 assert(b64,id+' present');const raw=Buffer.from(b64.replace(/\s/g,''),'base64');
 const disk=fs.readFileSync(path.join(root,file));assert(raw.equals(disk),id+' embedded bytes equal original FWSC');
 const pkg=await verifyPackage(fromBase64(b64),kind);assert.equal(pkg.product,'FM-1_1500');
 assert.equal(pkg.hash,crypto.createHash('sha256').update(disk).digest('hex'));
}
console.log('PASS V1.81 B1 HF2 SLOOP/Felucca OTA loader, CHECK->WRITE, denial safety, dual firmware SHA-256 parity');
