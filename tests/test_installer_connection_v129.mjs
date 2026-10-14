// SPDX-License-Identifier: GPL-3.0-only
// V1.29 installer connection regression: V1.26 must be accepted.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const html=fs.readFileSync(path.join(root,'ETCH-WebInstaller-V1.29.html'),'utf8');
const script=html.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
assert.ok(script, 'installer code exists');
const ctx={crypto:crypto.webcrypto,Uint8Array,ArrayBuffer,console,module:{exports:{}},setTimeout,clearTimeout};
ctx.globalThis=ctx;
vm.runInNewContext(script,ctx,{filename:'ETCH-V1.29 installer'});
const api=ctx.module.exports;
assert.equal(api.C.ID_ET,'FM-1_1029');
for(const version of [989,...Array.from({length:36},(_,i)=>994+i)]) {
  assert.equal(api.isSupportedEtchIdentity(`FM-1_${version}`),true,`FM-1_${version} must connect`);
}
for(const id of ['FM-1_988','FM-1_990','FM-1_991','FM-1_992','FM-1_993','FM-1_1030','FM-1_015','ota-FM-1_016','other_1026','FM-2_1026']) {
  assert.equal(api.isSupportedEtchIdentity(id),false,`${id} must remain blocked`);
}
assert.equal(api.isSupportedEtchIdentity('ota-FM-1_015'),true);
// Test the actual UI connection acceptance, not just the helper function.
const dom={};
const elem=id=>dom[id]??={dataset:{},classList:{toggle(){}},style:{},hidden:false,disabled:true,textContent:'',appendChild(){},scrollTop:0,scrollHeight:0};
const uiScript=script.replace('async function connectSelected(){','root.__testAcceptConnection=acceptConnection;\nasync function connectSelected(){');
assert.notEqual(uiScript,script);
const uiCtx={document:{getElementById:elem,querySelectorAll(){return[];},addEventListener(){},createElement(){return{textContent:''};}},setTimeout,clearTimeout,Date,Uint8Array,ArrayBuffer,console};
uiCtx.window=uiCtx;
vm.runInNewContext(uiScript,uiCtx,{filename:'ETCH-V1.29 UI'});
for(const id of ['FM-1_1026','FM-1_1027','FM-1_1029','ota-FM-1_015']) {
  elem('install').disabled=true;
  const device={id:{text:id,loader:id.startsWith('ota-')},link:{close(){throw new Error('accepted connection closed');}}};
  uiCtx.__testAcceptConnection(device);
  assert.equal(elem('install').disabled,false,`${id}: INSTALL must become enabled`);
  assert.match(elem('current-id').textContent,/FM-1_/);
}
let closed=false;
assert.throws(()=>uiCtx.__testAcceptConnection({id:{text:'FM-1_1030',loader:false},link:{close(){closed=true;}}}),/Do not flash/);
assert.equal(closed,true,'unsafe versions must have their link closed');
console.log('PASS V1.29 connection regression: V1.26, current, loader and blocked versions');
