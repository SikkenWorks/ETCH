import assert from 'node:assert/strict';
import fs from 'node:fs';
import crypto from 'node:crypto';
import vm from 'node:vm';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const releaseRoot=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const html=fs.readFileSync(path.join(releaseRoot,'ETCH-WebInstaller-V1.38-BEETLE-V042-COMPAT.html'),'utf8');
const existing=fs.readFileSync(path.join(releaseRoot,'ETCH-V1.38-OTA-RESTORE-FIX-EXPERIMENTAL.fwsc'));
const m=html.match(/<script id="firmware-embedded" type="application\/octet-stream">([\s\S]*?)<\/script>/);
assert(m,'embedded firmware');
const embedded=Buffer.from(m[1].replace(/\s/g,''),'base64');
assert.deepEqual(embedded,existing,'unchanged existing firmware package');
const source=html.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
assert(source);
assert.equal(crypto.createHash('sha256').update(embedded).digest('hex'),'095ef70ccf50a2677d794a0a9d939742c574cf42713c733c9ad7553c1fdbba8a');
assert.match(source,/function isSupportedBeetleIdentity/);
assert.match(source,/FM-1_2041/);
assert.match(source,/FM-1_2042/);
assert.match(source,/BEETLE V0.42.*UNVERIFIED SOURCE LOADER/);
assert.match(source,/requiresRestoreBridge/);
assert.doesNotMatch(source,/if\s*\(false\)\s*return.*CHECK/i);

const dom=new Map();
function elem(id){
 if(!dom.has(id))dom.set(id,{dataset:{},classList:{toggle(){}},style:{},textContent:'',innerText:'',hidden:false,disabled:false,appendChild(){},scrollTop:0,scrollHeight:0});
 return dom.get(id);
}
const ctx={
 document:{getElementById:elem,addEventListener(){},querySelectorAll(){return[];},createElement(){return {textContent:'',remove(){}};}},
 TextEncoder,Uint8Array,ArrayBuffer,Date,console,
 setTimeout,clearTimeout,
 crypto:crypto.webcrypto,atob:(x)=>Buffer.from(x,'base64').toString('binary'),
};
ctx.window=ctx;ctx.confirm=()=>true;
const injected=source.replace('async function connectSelected(){','window.__accept=acceptConnection;window.__choose=choosePackage;\nasync function connectSelected(){');
assert.notEqual(injected,source);
vm.runInNewContext(injected,ctx,{filename:'patched-etch-installer.js'});
const api=ctx.ETCHOta;
assert(api);
assert.equal(api.C.ID_ET,'FM-1_1038');
for(const id of ['FM-1_2031','FM-1_2032','FM-1_2033','FM-1_2041','FM-1_2042']){
 assert.equal(api.isSupportedBeetleIdentity(id),true,id+' recognized');
 assert.equal(api.isSupportedEtchIdentity(id),true,id+' allowed ETCH path');
 assert.equal(api.requiresRestoreBridge(id),true,id+' requires stock restore bridge');
}
for(const id of ['FM-1_2043','FM-1_2040','FM-1_9999','ota-FM-1_999']){
 assert.equal(api.isSupportedEtchIdentity(id),false,id+' refused');
}
assert.equal(api.isSupportedEtchIdentity('FM-1_1038'),true);
assert.equal(api.isSupportedEtchIdentity('FM-1_1027'),true);
assert.equal(api.requiresRestoreBridge('FM-1_1027'),true);
assert.equal(api.requiresRestoreBridge('FM-1_1038'),false);
const checked=await api.verifyPackage(Uint8Array.from(embedded),'etch');
assert.equal(checked.product,'FM-1_1038');
const corrupted=Uint8Array.from(embedded);corrupted[10000]^=1;
await assert.rejects(api.verifyPackage(corrupted,'etch'),/hash does not match/i);

function tryConnect(id,mode){
 ctx.__choose(mode);
 let closed=false;
 const dev={id:{text:id,loader:id.startsWith('ota-')},link:{close(){closed=true}}};
 let err=null;try{ctx.__accept(dev);}catch(e){err=e.message;}
 return {ok:!err,err,closed,status:elem('message').textContent};
}
assert.equal(tryConnect('FM-1_2041','etch').ok,true,'BEETLE 0.41 now connects');
assert.match(tryConnect('FM-1_2041','etch').status,/BEETLE.*CHECK/i);
assert.equal(tryConnect('FM-1_2033','etch').ok,true);
assert.equal(tryConnect('FM-1_2042','etch').ok,true,'BEETLE 0.42 recognized for ETCH update');
assert.equal(tryConnect('FM-1_2042','stock').ok,false,'BEETLE 0.42 cannot directly restore stock');
assert.equal(tryConnect('FM-1_2043','etch').ok,false,'unknown future BEETLE is blocked');
assert.equal(tryConnect('FM-1_2041','stock').ok,false,'stock from BEETLE still blocked');
assert.equal(tryConnect('FM-1_1027','stock').ok,false,'stock from old ETCH still blocked');
assert.equal(tryConnect('FM-1_1038','stock').ok,true,'V1.38 supports stock path');
assert.equal(tryConnect('FM-1_1038','etch').ok,true,'existing ETCH updates not broken');
assert.equal(tryConnect('ota-FM-1_015','etch').ok,true,'existing loader remains supported');
console.log('PASS: verified firmware unchanged, JS parsed, known BEETLE identities, safe stock gating, unknown IDs denied, corrupted package denied');
