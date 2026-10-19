import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import vm from 'node:vm';
const R=path.resolve(import.meta.dirname,'..');
const text=fs.readFileSync(path.join(R,'WebInstaller.html'),'utf8');
const variants=JSON.parse(fs.readFileSync(path.join(R,'release/V1.48-VARIANTS.json')));
const source=text.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
assert.ok(source,'embedded installer JS');
const context={crypto:crypto.webcrypto,Uint8Array,ArrayBuffer,TextEncoder,console,module:{exports:{}},setTimeout:(cb,ms)=>setTimeout(cb,1),clearTimeout};
context.globalThis=context;
vm.runInNewContext(source,context,{filename:'ETCH V1.48 installer'});
const api=context.module.exports;
assert.equal(api.C.ID_ET,'FM-1_1048');
const data={};
for(const [key,id,kind] of [['etch','firmware-embedded','etch'],['stock','firmware-embedded-stockhead','etch-stockhead']]){
  const b64=text.match(new RegExp(`<script id="${id}" type="application/octet-stream">\\s*([\\s\\S]*?)\\s*<\\/script>`))?.[1];
  assert.ok(b64,id);
  const raw=Buffer.from(b64.replace(/\s/g,''),'base64');
  const file=fs.readFileSync(path.join(R,variants[key].file));
  assert.deepEqual(raw,file);
  assert.equal(crypto.createHash('sha256').update(raw).digest('hex'),variants[key].sha256);
  const pkg=await api.verifyPackage(new Uint8Array(raw),kind);
  assert.equal(pkg.product,'FM-1_1048');
  data[key]=pkg;
}
assert.equal(api.recommendedHeadForSource('FM-1_015'),'stock');
assert.equal(api.recommendedHeadForSource('FM-1_915'),'stock');
assert.equal(api.recommendedHeadForSource('FM-1_989'),'etch');
assert.equal(api.recommendedHeadForSource('FM-1_1047'),'etch');
for (let v=1034;v<=1038;v++)assert.equal(api.recommendedHeadForSource('FM-1_'+v),'stock','archived V1.34–V1.38 original V15 protected bootheader');
assert.equal(api.pinnedSourceHead('FM-1_1038'),'stock');
assert.equal(api.pinnedSourceHead('FM-1_1033'),null);
assert.match(api.explainCheckFailure('FM-1_1038','etch',18),/KNOWN HEADER MISMATCH/);
assert.match(api.explainCheckFailure('FM-1_1038','stock',18),/error -5/);
assert.equal(api.isCompatibleFm1Identity('FM-1_015'),true);
for(const x of ['BEETLE_042','FELUCCA_916','SLOOP_022','X0X_014'])assert.equal(api.isCompatibleFm1Identity(x),true,x);
assert.equal(api.isCompatibleFm1Identity('ota-FM-1_015'),true);
assert.equal(api.isCompatibleFm1Identity('FM-2_015'),false);
assert.equal(api.isCompatibleFm1Identity('RANDOM_015'),false);
assert.equal(api.C.STOCK_HEAD_HASH,variants.stock.head_sha256);
assert.equal(api.C.KNOWN_HEAD_HASH,variants.etch.head_sha256);
const tampered=new Uint8Array(data.stock.raw);tampered[10000]^=0x10;
await assert.rejects(api.verifyPackage(tampered,'etch-stockhead'),/verification failed/);
await assert.rejects(api.verifyPackage(data.stock.raw,'etch'),/verification failed/);
await assert.rejects(api.verifyPackage(data.etch.raw,'etch-stockhead'),/verification failed/);
function makeReq(addr,n=8){const b=new Uint8Array(15);b.set([0,0x59,0x30,8,0,0,0]);
 b[7]=addr&255;b[8]=(addr>>>8)&255;b[9]=(addr>>>16)&255;b[10]=(addr>>>24)&255;
 b[11]=n&255;b[12]=(n>>>8)&255;b[13]=0;
 let sum=0;for(let i=6;i<14;i++)sum=(sum+b[i])&255;b[14]=(~sum)&255;
 return api.wrap(b);
}
function linkFor(finish,refuse=false){let requests=0;return {closed:false,sent:[],
 send(x){this.sent.push(x);return true;},get lost(){return false;},
 async next(){requests++;if(refuse)return null;if(requests===1)return makeReq(16,16);
 if(requests===2)return makeReq(finish,8);return null;},
 async close(){this.closed=true;}};}
for(const [key,src] of [['etch','FM-1_1047'],['stock','FM-1_015'],['stock','FM-1_915']]){
 console.log('TEST INSTALL',key,src); const checked=linkFor(api.C.FINISH_CHECK), loader=linkFor(api.C.FINISH_WRITE), boot=linkFor(0);
 const eng=new api.Engine(null,(message)=>{if(process.env.DEBUG148)console.log('LOG',message)},(step,_,label)=>{if(process.env.DEBUG148)console.log('STEP',step,label)});
 eng.waitFor=async pred=>{
   if(pred({loader:true,text:'ota-FM-1_015'}))return {id:{loader:true,text:'ota-FM-1_015'},link:loader};
   if(pred({loader:false,text:'FM-1_1048'}))return {id:{loader:false,text:'FM-1_1048'},link:boot};
   return null;
 };
 await eng.install({id:{loader:false,text:src},link:checked},data[key]);
 assert(checked.closed && loader.closed && boot.closed);
 assert(checked.sent.length>=3 && loader.sent.length>=3);
}
const blocked=linkFor(api.C.FINISH_CHECK,true);
const eng=new api.Engine(null,(message)=>{if(process.env.DEBUG148)console.log('LOG',message)},(step,_,label)=>{if(process.env.DEBUG148)console.log('STEP',step,label)});let loaderChecked=false;
eng.waitFor=async()=>{loaderChecked=true;return null;};
await assert.rejects(eng.install({id:{loader:false,text:'FM-1_915'},link:blocked},data.stock),/did not approve/);
assert.equal(loaderChecked,false);
// Unknown loader outside the approved stock / B1-HF2 9xx families is blocked.
await assert.rejects(eng.install({id:{loader:true,text:'ota-FM-1_777'},link:linkFor(api.C.FINISH_WRITE)},data.stock),/Unknown OTA loader/);
assert.equal(fs.readFileSync(path.join(R,'firmware/fm1-native/native_fx.c')).equals(fs.readFileSync(path.join(R,'tests/reference/V1.45-native_fx.c'))),true);
console.log('PASS V1.48 dual-header package integrity, source selection, protected OTA CHECK and loader identity');
