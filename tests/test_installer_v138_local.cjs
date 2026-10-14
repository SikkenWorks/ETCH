const path=require('node:path');
const ROOT=path.resolve(__dirname,'..');
const fs=require('node:fs');
const vm=require('node:vm');
const assert=require('node:assert/strict');
const crypto=require('node:crypto');
const html=fs.readFileSync(path.join(ROOT,'ETCH-WebInstaller-V1.38-OTA-RESTORE-FIX.html'),'utf8');
const match=html.match(/<script id="firmware-embedded" type="application\/octet-stream">([\s\S]*?)<\/script>/);
assert(match);
const embedded=Buffer.from(match[1].replace(/\s+/g,''),'base64');
const file=fs.readFileSync(path.join(ROOT,'ETCH-FM1-V1.38-OTA-RESTORE-FIX-EXPERIMENTAL.fwsc'));
assert.deepEqual(embedded,file);
const code=html.match(/<script>(\/\* SPDX-License-Identifier: GPL-3\.0-only[\s\S]*?)<\/script>/)[1];
const ctxt={module:{exports:{}},crypto:crypto.webcrypto,Uint8Array,TextEncoder,atob};
vm.runInNewContext(code,ctxt);
const api=ctxt.module.exports;
(async()=>{
 const p=await api.verifyPackage(new Uint8Array(file),'etch');
 assert.equal(p.product,'FM-1_1038');
 assert.equal(p.hash,crypto.createHash('sha256').update(file).digest('hex'));
 if(process.env.FM1_STOCK_V15){
 const stock=await api.verifyPackage(new Uint8Array(fs.readFileSync(process.env.FM1_STOCK_V15)),'stock');
 assert.equal(stock.product,'FM-1_015');
 assert.equal(stock.hash,api.C.HASH_STOCK);
 }
 assert.equal(api.requiresRestoreBridge('FM-1_1027'),true);
 assert.equal(api.requiresRestoreBridge('FM-1_1037'),true);
 assert.equal(api.requiresRestoreBridge('FM-1_1038'),false);
 assert.equal(api.isSupportedEtchIdentity('FM-1_1038'),true);
 const bad=new Uint8Array(file);bad[1000]^=1;
 await assert.rejects(async()=>api.verifyPackage(bad,'etch'),/hash/i);
 console.log('PASS: ETCH V1.38 installer firmware parity, SHA, stock verification, version gates, changed-package rejection');
})().catch(e=>{console.error(e);process.exitCode=1});
