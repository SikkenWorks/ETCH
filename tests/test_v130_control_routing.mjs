// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
const root=new URL('../',import.meta.url);
const fw=fs.readFileSync(new URL('firmware/fm1-native/bringup.c',root),'utf8');
const codec=fs.readFileSync(new URL('firmware/fm1-native/native_project_codec.c',root),'utf8');
const h=fs.readFileSync(new URL('firmware/fm1-native/native_advanced_controls_v130.h',root),'utf8');
for (const route of ['organic','living','matrix']) assert.match(fw,new RegExp('etch_v130_'+route+'\\('));
for (const key of ['drift','rate','phase','variance','depth','color','vary'])assert.ok(h.includes('->'+key),'Independent control '+key);
assert.ok(!fw.includes('etch_encoder_pct(native_project_cfg.living.depth'),'LIVING must not use 16-bit conversion');
assert.ok(fw.includes('native_preset_dsp_capture(slot)')&&fw.includes('native_preset_dsp_restore(native_patch_slot)'));
assert.ok(codec.includes('V26MAGIC')&&codec.includes('etch_dsp_bound(s)'));
assert.ok(fw.includes('native_bpm_overlay_until')&&fw.includes('etch_v130_bpm(native_tempo,preset)'));
assert.ok(!fw.includes('label("K1 SLOT K2 SRC K3 DST K4 AMT"'),'Remove redundant MOD MATRIX footer');
console.log('PASS V1.30 advanced routing / per-preset persistence / BPM overlay');
