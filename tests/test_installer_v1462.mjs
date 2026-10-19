import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import crypto from 'node:crypto';
const root=path.resolve(import.meta.dirname,'..');
const manifest=JSON.parse(fs.readFileSync(path.join(root,'RELEASE-MANIFEST.json')));
const html=fs.readFileSync(path.join(root,'WebInstaller.html'),'utf8');
const fwsc=fs.readFileSync(path.join(root,manifest.firmware_file));
const b64=html.match(/<script id="firmware-embedded" type="application\/octet-stream">\s*([\s\S]*?)\s*<\/script>/);
assert.ok(b64,'installer must include actual firmware');
assert.deepEqual(Buffer.from(b64[1].replace(/\s/g,''),'base64'),fwsc);
assert.equal(manifest.firmware_identity,'FM-1_1462');
assert.equal(crypto.createHash('sha256').update(fwsc).digest('hex'),manifest.fwsc_sha256);
const code=html.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
assert.ok(code);
const context={crypto:crypto.webcrypto,Uint8Array,ArrayBuffer,TextEncoder,console,module:{exports:{}},setTimeout,clearTimeout};context.globalThis=context;
vm.runInNewContext(code,context,{filename:'ETCH V1.46.2 OTA verifier'});
const {verifyPackage,isSupportedEtchIdentity}=context.module.exports;
const verify=await verifyPackage(new Uint8Array(fwsc),'etch');
assert.equal(verify.product,'FM-1_1462');
assert.equal(verify.hash,manifest.fwsc_sha256);
for(const id of ['FM-1_1045','FM-1_1046','FM-1_1461','FM-1_1462','FM-1_1401','FM-1_1044']){
 assert.ok(isSupportedEtchIdentity(id),'source accepted: '+id);
}
assert.equal(isSupportedEtchIdentity('FM-1_777'),false);
const corrupted=new Uint8Array(fwsc);corrupted[41000]^=1;
await assert.rejects(verifyPackage(corrupted,'etch'),/hash does not match/);
assert.deepEqual(fs.readFileSync(path.join(root,'firmware/fm1-native/native_fx.c')),
                     fs.readFileSync(path.join(root,'tests/reference/V1.45-native_fx.c')),
                     'crashing V1.46 FX must never reappear');
console.log('PASS V1.46.2 installer, physical-version allowlist, CRC/hash, corrupt packet refusal, original FX');
