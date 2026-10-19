import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import crypto from 'node:crypto';
const root=path.resolve(import.meta.dirname,'..');
const text=fs.readFileSync(path.join(root,'WebInstaller.html'),'utf8');
const src=text.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
assert(src,'embedded OTA JS');
const c={crypto:crypto.webcrypto,Uint8Array,ArrayBuffer,TextEncoder,console,module:{exports:{}},setTimeout,clearTimeout};c.globalThis=c;
vm.runInNewContext(src,c,{filename:'V1.48.2 OTA'});
const a=c.module.exports;
for(let v=1034;v<=1038;v++){
 const identity='FM-1_'+v;
 assert.equal(a.pinnedSourceHead(identity),'stock',identity);
 assert.equal(a.recommendedHeadForSource(identity),'stock',identity);
 assert.match(a.explainCheckFailure(identity,'etch',18),/KNOWN HEADER MISMATCH/);
}
for(const v of [989,1040,1041,1042,1043,1044,1045,1046,1047,1048,1401,1461,1462])
 assert.equal(a.recommendedHeadForSource('FM-1_'+v),'etch','archived ETCH head '+v);
assert.equal(a.pinnedSourceHead('FM-1_1033'),null,'do not pretend unknown historical pin');
assert.equal(a.recommendedHeadForSource('FELUCCA_912'),'stock','third party remains a CHECK-only candidate');
const head=fs.readFileSync(path.join(root,'tests/fixtures/v138_stock_head.bin'));
assert.equal(crypto.createHash('sha256').update(head).digest('hex'),a.C.STOCK_HEAD_HASH);
for(const [variant,tag] of [['stock','firmware-embedded-stockhead'],['etch','firmware-embedded']]){
 const match=text.match(new RegExp(`<script id="${tag}" type="application/octet-stream">\\s*([\\s\\S]*?)\\s*<\\/script>`));
 assert(match,tag);
 const raw=Buffer.from(match[1].replace(/\s/g,''),'base64');
 const file=fs.readFileSync(path.join(root,`release/ETCH-FM1-V1.50-${variant.toUpperCase()}-HEAD.fwsc`));
 assert.deepEqual(raw,file,'embedded '+variant+' package');
}
assert.match(text,/pinnedSourceHead\(id\)/,'UI checks known incompatible explicit manual selection');
assert.match(text,/selected==='auto'\?recommendedHeadForSource\(id\)/,'AUTO uses pinned selector');
console.log('PASS V1.48.2 exact historical header map, installer selection, both firmware packages');

