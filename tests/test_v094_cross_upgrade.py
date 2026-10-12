#!/usr/bin/env python3
"""Reproduce the V0.93 regression and validate the V0.94 OTA-CHECK fix offline."""
from pathlib import Path
import hashlib,subprocess,tempfile,sys
root=Path(__file__).resolve().parents[1]
release=root.parent
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as F
reference=release/'reference/ETCH-V0.89-FULL-FM1-EXPERIMENTAL.fwsc'
newpackage=release/'reference/ETCH-V0.94-V089-COMPAT-EXPERIMENTAL.fwsc.lab'
app=root/'build/fm1-native-storage-updater/app.bin'
assets=F.extract_stock_assets(reference)
plainflash=F.build_flash_image_from_assets(app.read_bytes(),assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
installed=F.parse_ufw(reference.read_bytes())['entries'][0]['data'][:0x4000]
assert [i for i in range(0x4000) if installed[i]!=plainflash[i]]==[0x20,0x21,0x22,0x23]
with tempfile.TemporaryDirectory(prefix='etch-v094-ota-cross-') as tmp:
    t=Path(tmp)
    t.joinpath('original-V093-style.fwsc.lab').write_bytes(F.build_fwsc(plainflash,assets['ota'],'FM-1_994'))
    t.joinpath('V089-head.bin').write_bytes(installed)
    binary=t/'ota_test'
    subprocess.run(['gcc','-O2','-std=c11',str(root/'tests/test_native_ota.c'),str(root/'firmware/fm1-native/native_digest.c'),'-o',str(binary)],check=True)
    rejected=subprocess.run([str(binary),str(t/'original-V093-style.fwsc.lab'),str(t/'V089-head.bin')],capture_output=True,text=True)
    accepted=subprocess.run([str(binary),str(newpackage),str(t/'V089-head.bin')],capture_output=True,text=True)
    print('Unfixed image:',rejected.stdout.strip())
    print('V0.94 fixed image:',accepted.stdout.strip())
    assert rejected.returncode!=0 and 'CROSS_OTA_RESULT=-5 REQUESTS=18 COMMITTED=0' in rejected.stdout
    assert accepted.returncode==0 and 'CROSS_OTA_RESULT=0 REQUESTS=65 COMMITTED=1 BOOT_HEAD_UNCHANGED=1' in accepted.stdout
print('PASS: 0.89 -> unfixed refuses at request 18; 0.89 -> fixed reaches commit with identical protected boot head')
