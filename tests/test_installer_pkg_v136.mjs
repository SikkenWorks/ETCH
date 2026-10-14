// SPDX-License-Identifier: GPL-3.0-only
// Exact public standalone .fwsc, browser-installer payload, identity, CRC and tamper rejection.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const html=fs.readFileSync(path.join(root,'ETCH-WebInstaller-V1.36.html'),'utf8');
const fwsc=fs.readFileSync(path.join(root,'ETCH-FM1-V1.36.fwsc'));
const code=html.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
const payload=html.match(/<script id="firmware-embedded" type="application\/octet-stream">\s*([\s\S]*?)\s*<\/script>/)?.[1];
assert.ok(code && payload,'installer and embedded firmware present');
const embedded=Buffer.from(payload.replace(/\s/g,''),'base64');
assert.deepEqual(fwsc,embedded,'downloadable .fwsc MUST BE byte-identical to browser payload');
const hash=crypto.createHash('sha256').update(fwsc).digest('hex');
const manifest=JSON.parse(fs.readFileSync(path.join(root,'RELEASE-MANIFEST.json'),'utf8'));
assert.equal(hash,'dca1c955b518840d5fabe9bee1208cedce6d83d7d7f476336ac3786868ada591');
assert.equal(manifest.firmware_identity,'FM-1_1037'); // active V1.37; this tests archived V1.36
const ctx={crypto:crypto.webcrypto,Uint8Array,ArrayBuffer,TextEncoder,console,module:{exports:{}},setTimeout,clearTimeout};
ctx.globalThis=ctx;
vm.runInNewContext(code,ctx,{filename:'ETCH installer V1.36'});
const api=ctx.module.exports;
const pkg=await api.verifyPackage(new Uint8Array(fwsc),'etch');
assert.equal(pkg.product,'FM-1_1036');
assert.equal(pkg.hash,hash);
assert.equal(pkg.image.length,fwsc.length-20);
assert.equal(api.isSupportedEtchIdentity('FM-1_1033'),true);
assert.equal(api.isSupportedEtchIdentity('FM-1_1036'),true);
assert.equal(api.isSupportedEtchIdentity('ota-FM-1_015'),true);
assert.equal(api.isSupportedEtchIdentity('FM-1_1034'),true);
assert.equal(api.isSupportedEtchIdentity('FM-1_1035'),true);
assert.equal(api.isSupportedEtchIdentity('FM-1_1037'),false);
const bad=new Uint8Array(fwsc);bad[40000]^=1;
await assert.rejects(api.verifyPackage(bad,'etch'),/hash does not match/);
console.log('PASS V1.36 .fwsc byte-identical with installer, identity, compatibility and corruption guards');
