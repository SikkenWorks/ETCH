import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';
import crypto from 'node:crypto';

const here=path.dirname(fileURLToPath(import.meta.url));
const root=path.resolve(here,'..');
// Bind to the manifest version, never the first stale installer in a folder.
const manifest=JSON.parse(fs.readFileSync(path.join(root,'RELEASE-MANIFEST.json'),'utf8'));
const filename=`ETCH-WebInstaller-V${manifest.version}.html`;
assert.ok(fs.existsSync(path.join(root,filename)),'Versioned installer must match release manifest');
const version=filename.match(/V(\d+)\.(\d+)\.html$/);
const expectedId=`FM-1_${1000+Number(version[2])}`;
assert.equal(manifest.firmware_identity,expectedId,'update firmware identity when version changes');
const nativeSource=fs.readFileSync(path.join(root,'firmware/fm1-native/bringup.c'),'utf8');
assert.ok(nativeSource.includes(`#define FELUCCA_ID "${expectedId}"`),
  'compiled app firmware identity must agree with installer (V1.23 regression)');
const appPath=path.join(root,'build/fm1-native-storage-updater-p05/app.bin');
if(fs.existsSync(appPath))assert.ok(fs.readFileSync(appPath).includes(Buffer.from(expectedId)),
  'crossbuilt app binary must contain current firmware identity');
const html=fs.readFileSync(path.join(root,filename),'utf8');
const embedded=html.match(/<script id="firmware-embedded" type="application\/octet-stream">\s*([\s\S]*?)\s*<\/script>/);
assert.ok(embedded,'firmware must be embedded');
const firmware=Buffer.from(embedded[1].replace(/\s/g,''),'base64');
assert.equal(firmware.length,manifest.experimental_firmware_bytes);
assert.equal(crypto.createHash('sha256').update(firmware).digest('hex'),manifest.experimental_firmware_sha256);
const code=html.match(/<script>\s*(\/\* SPDX-License-Identifier:[\s\S]*?)<\/script>/)?.[1];
assert.ok(code,'main OTA script found');
assert.match(code,/ota-FM-1_015/,'known OTA loader identity must be allowed');
assert.match(code,new RegExp(expectedId),'current ETCH identity must be allowed');
assert.match(html,/github\.com\/SikkenWorks\/ETCH/,'credits link stays intact');

// Exercise the actual acceptConnection function without a real device.
const exposed=code.replace('async function connectSelected(){','root.__testAcceptConnection=acceptConnection;\nasync function connectSelected(){');
assert.notEqual(code,exposed,'test hook injected');
const dom={};
function elem(id){return dom[id]??=(id==='log'?{appendChild(){},scrollTop:0,scrollHeight:0}:{dataset:{},classList:{toggle(){}},style:{},hidden:false,disabled:false,textContent:''});}
const ctx={
    document:{getElementById:elem,querySelectorAll(){return[];},createElement(){return{textContent:''};},addEventListener(){}},
    setTimeout,clearTimeout,TextEncoder,Uint8Array,ArrayBuffer,Date,console,
};
ctx.window=ctx;
vm.runInNewContext(exposed,ctx,{filename:'installer.js'});
assert.equal(typeof ctx.__testAcceptConnection,'function');
const allow=(id)=>{
    let closed=false;
    const dev={id:{text:id,loader:id.startsWith('ota-')},link:{close(){closed=true;}}};
    try{ctx.__testAcceptConnection(dev);return {ok:true,closed,installDisabled:elem('install').disabled,message:elem('message').textContent};}
    catch(e){return {ok:false,closed,error:e.message};}
};
assert.equal(allow(expectedId).ok,true,'same-version reinstall should be allowed');
assert.equal(allow('ota-FM-1_015').ok,true,'known loader should be recognized');
assert.match(allow('ota-FM-1_015').message,/previously installed application cannot be verified/);
assert.equal(allow('FM-1_777').ok,false,'unknown running image must be rejected');
assert.equal(allow('ota-FM-1_016').ok,false,'unknown loader must be rejected');
console.log('Installer OTA/current-version compatibility: PASS');
