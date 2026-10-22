#!/usr/bin/env python3
"""Fail CI on accidental ETCH FM1 product-ID drift after V1.50.

V1.60+ is a version label, never a new firmware/USB/package identity.
"""
from pathlib import Path
import json, re, sys
import fm1_fwsc as fw
root=Path(__file__).resolve().parents[1]
expected='FM-1_1500'
checks={
 'native USB/SysEx firmware identity':(root/'firmware/fm1-native/bringup.c','FELUCCA_ID "'+expected+'"'),
 'packaging default':(root/'tools/fm1_fwsc.py','DEFAULT_PRODUCT = "'+expected+'"'),
 'v160 packer':(root/'tools/build_v160_packages.py',"PRODUCT = '"+expected+"'"),
 'WebInstaller identity':(root/'WebInstaller.html',"ID_ET: '"+expected+"'"),
 'README identity':(root/'README.md',expected),
}
for label,(file,needle) in checks.items():
    if needle not in file.read_text():sys.exit(f'FAIL {label}: package identity changed')
    print('PASS',label,expected)
m=json.loads((root/'RELEASE-MANIFEST.json').read_text())
if m['firmware_identity']!=expected:sys.exit('FAIL release manifest product identity drift')
for key,obj in m['packages'].items():
    target=root/obj['file']
    r,errors=fw.verify_fwsc(target)
    if errors or r['product']!=expected:sys.exit(f'FAIL {key} package identity or structure: {errors}')
    print('PASS',key,'package',expected)
print('PASS unchanged identity contract for current and future V1.60+ revisions')
