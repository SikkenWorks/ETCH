// SPDX-License-Identifier: GPL-3.0-only
// ETCH V1.29: Guard against stale advanced parameter footers / bad PC6 wiring.
import assert from 'node:assert/strict';
import fs from 'node:fs';
const fw=fs.readFileSync(new URL('../firmware/fm1-native/bringup.c',import.meta.url),'utf8');
const audio=fs.readFileSync(new URL('../firmware/fm1-native/audio_target.c',import.meta.url),'utf8');
const footer=fw.match(/const uint16_t visible_values\[\]=\{([\s\S]*?)\n \};/)[1];
for(const field of [
 'organic.drift','organic.rate','organic.phase','organic.variance',
 'living.depth','living.rate','living.color','living.vary',
 'matrix.route[native_matrix_slot].source','matrix.route[native_matrix_slot].target',
 'matrix.route[native_matrix_slot].amount','native_matrix_slot',
 'native_harmonic','native_editor.revision',
 'native_blend_a','native_blend_b','native_blend_pct','native_blend_on',
 'native_riff2.riff_densities','native_riff2.riff_mutations',
 'native_riff2.riff_variants','native_riff_seeds'
])assert.ok(footer.includes(field),`Changing ${field} must redraw the numeric parameter footer`);
const uiEditor=fw.match(/else if\(native_page==ETCH_UI_SPECTRAL(?: && native_mono_mode)?\)\{([\s\S]*?)\n    \}/g)?.at(-1)??'';
for(const knob of ['dm','dd','up','down'])assert.match(uiEditor,new RegExp(`\\b${knob}\\b`),`SPECTRAL ${knob} handled`);
for(const page of ['MATRIX','LIVING','ORGANIC','PATCH_BLEND']){
 const i=Math.max(fw.lastIndexOf(`else if(native_page==ETCH_UI_${page}){`),fw.lastIndexOf(`else if(native_page==ETCH_UI_${page} && native_mono_mode){`));
 assert.ok(i>0,`${page} has a real encoder handler`);
 const body=fw.slice(i,i+1400);
 for(const knob of ['dm','dd','up','down'])assert.match(body,new RegExp(`\\b${knob}\\b`),`${page} ${knob} handled`);
}
assert.match(fw,/if\(!native_blend_on\)native_blend_pending=0u/,
 'Choosing source A/B while PATCH BLEND is OFF must not apply an edit');
assert.match(fw,/if\(native_blend_audio_busy\(\)\)native_blend_pending=1u/,
 'A busy audio engine must postpone PATCH BLEND waveform compilation');
assert.match(fw,/native_analog_gate_test=\(uint8_t\)\(!safe && !!\(fm1_in.buttons&\(1u<<7\)\)\)/,
 'GLO held at boot enables the diagnostic only when not in REC safe mode');
assert.match(audio,/if\(native_analog_gate_test\)return;/,
 'Silent diagnostic prevents PC6 high but does not skip fm1_audio_init');
assert.match(audio,/fm1_audio_output_enable\(1u\)/,
 'Normal operation still enables audio output');
assert.match(fw,/K4 PHASE/, 'MOD WAVE explains binary target selection');
assert.match(fw,/K4 MORPH/, 'MOD WAVE explains binary target selection');
console.log('PASS V1.29 advanced page knob publication, safe blend selection, PC6 diagnostic + MOD WAVE legends');
