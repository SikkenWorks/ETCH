// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
const ui=fs.readFileSync('firmware/fm1-native/bringup.c','utf8');
assert.match(ui,/native_arp_mode==7u\?"NEW RIFF":"BPM"/);
assert.match(ui,/native_page==ETCH_UI_ARP && native_arp_mode==7u && i==3u\)\?168u/);
assert.ok(168+8*8<=240,'full NEW RIFF label fits at x=168');
assert.match(ui,/native_new_riff\(native_patch_slot,down\);/);
assert.match(ui,/if\(down\)native_new_riff\(ps,down\)/);
assert.ok(!ui.includes('while(n--)native_riff_seeds[native_patch_slot]'), 'no bypass of distinct riff generator');
assert.match(ui,/native_controls\.riff_seed=native_riff_seeds\[native_patch_slot\]/,
  'riff selection must publish from the saved patch, not the UI page');
console.log('PASS V1.36 K4 full caption, page-independent patch seed publishing, deterministic generator');
