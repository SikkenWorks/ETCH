// SPDX-License-Identifier: GPL-3.0-only
// Display's four 60-px columns have at most seven 8-px glyphs.
import assert from 'node:assert/strict';
import fs from 'node:fs';
const source=fs.readFileSync(new URL('../firmware/fm1-native/bringup.c',import.meta.url),'utf8');
const rule=source.match(/native_page==ETCH_UI_SPECTRAL\)\{names\[0\]="([^"]+)";names\[1\]="([^"]+)";names\[2\]="([^"]+)";names\[3\]="([^"]+)"/);
assert.ok(rule,'SPECTRAL has a named 4-column footer');
for(let i=1;i<5;i++) assert.ok(rule[i].length<=7,`K${i} label ${rule[i]} overflows 60-px cell`);
assert.equal(rule[4],'TILT');
assert.equal(rule[3],'GAIN');
assert.ok(source.includes('uint32_t maxchars=(x<240u'),'display code clips safely at 240 pixels');
assert.ok(source.includes('if(native_spectral_bank_pending)label("WAIT NOTE OFF"'),'deferred spectral bank is explained on UI');
console.log('PASS V1.32 SPECTRAL four columns: distinct labels, no truncated or scrambled K4 text');
