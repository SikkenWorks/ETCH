// SPDX-License-Identifier: GPL-3.0-only
// V1.33: actually exercise the browser installer's package verifier against
// the candidate firmware, not merely compare a manifest string.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const html = fs.readFileSync(path.join(root,'ETCH-WebInstaller-V1.33.html'),'utf8');
const source = html.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
const payload=html.match(/<script id="firmware-embedded" type="application\/octet-stream">\s*([\s\S]*?)\s*<\/script>/)?.[1];
assert.ok(source && payload);
const firmware=Buffer.from(payload.replace(/\s/g,''),'base64');
const context={ crypto:crypto.webcrypto, Uint8Array, ArrayBuffer, TextEncoder, console,
                module:{exports:{}}, setTimeout, clearTimeout};
context.globalThis=context;
vm.runInNewContext(source,context,{filename:'etch-installer.js'});
const api=context.module.exports;
const parsed=await api.verifyPackage(new Uint8Array(firmware),'etch');
assert.equal(parsed.product,'FM-1_1033');
assert.equal(parsed.hash,crypto.createHash('sha256').update(firmware).digest('hex'));
assert.equal(parsed.image.length,firmware.length-20);
const damaged=new Uint8Array(firmware);damaged[40000]^=1;
await assert.rejects(api.verifyPackage(damaged,'etch'),/hash does not match/);
console.log('PASS V1.33 embedded candidate verifies real product/hash/header/OTA and rejects corruption');
