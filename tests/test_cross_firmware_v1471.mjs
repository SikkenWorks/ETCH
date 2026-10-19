import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import vm from 'node:vm';

const root=path.resolve(import.meta.dirname,'..');
const html=fs.readFileSync(path.join(root,'WebInstaller.html'),'utf8');
const manifest=JSON.parse(fs.readFileSync(path.join(root,'RELEASE-MANIFEST.json')));
const image=new Uint8Array(fs.readFileSync(path.join(root,manifest.firmware_file)));
const embedded=html.match(/<script id="firmware-embedded" type="application\/octet-stream">\s*([\s\S]*?)\s*<\/script>/)?.[1];
assert.ok(embedded,'embedded firmware');
assert.deepEqual(Buffer.from(embedded.replace(/\s/g,''),'base64'),Buffer.from(image));
const source=html.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
assert.ok(source,'OTA JS code');
const context={crypto:crypto.webcrypto,Uint8Array,ArrayBuffer,TextEncoder,console,module:{exports:{}},setTimeout:(cb,ms)=>setTimeout(cb,1),clearTimeout};
context.globalThis=context;
vm.runInNewContext(source,context,{filename:'ETCH installer'});
const api=context.module.exports;
const pkg=await api.verifyPackage(image,'etch');
assert.equal(pkg.product,'FM-1_1047');
const incoming=['FM-1_015','FM-1_093','FM-1_101','FM-1_777','FM-1_915','FM-1_989','FM-1_1045','FM-1_1046','FM-1_1047','FM-1_1401','FM-1_1461','FM-1_1462','ota-FM-1_015'];
for(const id of incoming) assert.equal(api.isCompatibleFm1Identity(id),true,`cross-firmware connect: ${id}`);
for(const id of ['FM-2_015','BEETLE_007','ota-FM-2_015','ota-FM-1_999','USB-MIDI','', 'FM-1_1047garbage','FM-1_-1']) assert.equal(api.isCompatibleFm1Identity(id),false,`unsafe identity blocked: ${id}`);
assert.equal(api.isCrossFirmwareSource('FM-1_915'),true);
assert.equal(api.isCrossFirmwareSource('FM-1_1047'),false);
assert.equal(api.isCrossFirmwareSource('ota-FM-1_015'),false);
assert.equal(api.Engine.known('BEETLE'),true);
assert.equal(api.Engine.known('SLOOP'),true);
assert.equal(api.Engine.known('Felucca'),true);
assert.ok(source.includes("!isCompatibleFm1Identity(device.id.text)"),'connect uses broad FM1 match');

// Emulate device-side two-stage check; unlike a naive allowlist bypass,
// the firmware must explicitly request CHECK finish before WRITE can start.
function makeRequest(addr,n=8){
 const b=new Uint8Array(15);b.set([0,0x59,0x30,8,0,0,0]);
 b[7]=addr&255;b[8]=(addr>>>8)&255;b[9]=(addr>>>16)&255;b[10]=(addr>>>24)&255;
 b[11]=n&255;b[12]=(n>>>8)&255;b[13]=0;
 let sum=0;for(let i=6;i<14;i++)sum=(sum+b[i])&255;b[14]=(~sum)&255;
 return api.wrap(b);
}
function linkOf(finish,refuse=false){
 let requests=0;const sent=[];
 return {
  sent,closed:false,lost:false,
  send(packet){sent.push(Array.from(packet));return true;},
  async next(){requests++;if(refuse)return null;if(requests===1)return makeRequest(16,16);if(requests===2)return makeRequest(finish,8);return null;},
  async close(){this.closed=true;}
 };
}
const check=linkOf(api.C.FINISH_CHECK);
const loader=linkOf(api.C.FINISH_WRITE);
const boot=linkOf(0);
const stages=[];
const engine=new api.Engine(null, s=>stages.push(String(s)), ()=>{});
engine.waitFor=async predicate=>{
 if(predicate({loader:true,text:'ota-FM-1_015'}))return {id:{loader:true,text:'ota-FM-1_015'},link:loader};
 if(predicate({loader:false,text:'FM-1_1047'}))return {id:{loader:false,text:'FM-1_1047'},link:boot};
 return null;
};
await engine.install({id:{loader:false,text:'FM-1_915'},link:check},pkg);
assert.ok(check.closed && loader.closed && boot.closed);
assert.ok(check.sent.length>=3 && loader.sent.length>=3, 'both CHECK and WRITE occur');
assert.ok(stages.some(t=>t.includes('CHECK stage')), 'CHECK stage confirmation');
// Negative path: an incompatible source that never approves the CHECK cannot flash.
const refused=linkOf(api.C.FINISH_CHECK,true);
const blockedEngine=new api.Engine(null,()=>{},()=>{});
let loaderCalled=false;blockedEngine.waitFor=async()=>{loaderCalled=true;throw Error('illegal loader transition');};
await assert.rejects(blockedEngine.install({id:{loader:false,text:'FM-1_777'},link:refused},pkg),/did not approve.*OTA CHECK/);
assert.equal(loaderCalled,false);
assert.equal(refused.closed,true);
// No silent package tampering.
const broken=image.slice();broken[40960]^=0xff;
await assert.rejects(api.verifyPackage(broken,'etch'),/hash does not match/);
assert.deepEqual(fs.readFileSync(path.join(root,'firmware/fm1-native/native_fx.c')), fs.readFileSync(path.join(root,'tests/reference/V1.45-native_fx.c')));
console.log('PASS cross-firmware FM1 identity, checked OTA/loader transfer, rejection, package integrity, FX baseline');
