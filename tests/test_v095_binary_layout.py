#!/usr/bin/env python3
"""Confirm actual V0.95 target build and packages enable PROJECTS (no MIDI sent)."""
from pathlib import Path
import json,sys,hashlib,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
release=root.parent
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as F
app=root/'build/fm1-native-storage-updater-p05/app.bin'
opts=json.loads((app.parent/'native-build-options.json').read_text())
data=app.read_bytes()
assert opts['storage_writes'] and opts['ota_writes'] and opts['multi_project_experimental'],opts
assert opts['version']=='0.95' and opts['product']=='FM-1_995'
for marker in (b'V095',b'FM-1_995',b'PROJECT BROWSER'):
 assert marker in data,marker
for stale in (b'V090',b'PROJECTS DISABLED',b'FM-1_994'):
 assert stale not in data,stale
ui=(root/'firmware/fm1-native/bringup.c').read_text()
assert 'label("V090"' not in ui and ui.count('label(ETCH_UI_VERSION')==5
new=F.parse_ufw((release/'ETCH-V0.95-V089-BOOT-COMPAT-EXPERIMENTAL.fwsc.lab').read_bytes())
old=F.parse_ufw((release/'reference/ETCH-V0.94-V089-COMPAT-EXPERIMENTAL.fwsc.lab').read_bytes())
assert old['product']=='FM-1_994' and new['product']=='FM-1_995'
flash_new=next(e['data'] for e in new['entries'] if e['name']=='flash.bin')
flash_old=next(e['data'] for e in old['entries'] if e['name']=='flash.bin')
assert flash_new[:0x4000]==flash_old[:0x4000]
assert len(flash_new)==len(flash_old)==0x93000
assert next(e['data'] for e in new['entries'] if e['name']=='ota.bin')==next(e['data'] for e in old['entries'] if e['name']=='ota.bin')
print('PASS V0.95 binary has PROJECT BROWSER and V095, no V090/disabled; P05 enabled by default')
print('PASS V0.94 -> V0.95 shared pinned 16KiB header and identical OTA loader, no storage included in image')
with tempfile.TemporaryDirectory() as temp:
 t=Path(temp);(t/'installed-head.bin').write_bytes(flash_old[:0x4000]);bin=t/'ota_test'
 subprocess.run(['gcc','-O2','-std=c11',str(root/'tests/test_native_ota.c'),str(root/'firmware/fm1-native/native_digest.c'),'-o',str(bin)],check=True)
 ret=subprocess.run([str(bin),str(release/'ETCH-V0.95-V089-BOOT-COMPAT-EXPERIMENTAL.fwsc.lab'),str(t/'installed-head.bin')],capture_output=True,text=True)
 assert ret.returncode==0 and 'CROSS_OTA_RESULT=0 REQUESTS=65 COMMITTED=1 BOOT_HEAD_UNCHANGED=1' in ret.stdout,ret.stdout+ret.stderr
 print('PASS V0.94 simulated OTA stage-1 CHECK -> V0.95 accepted, 65 reads, protected head unchanged')
