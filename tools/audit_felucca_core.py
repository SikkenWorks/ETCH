#!/usr/bin/env python3
"""Verify preserved HAL/boot files against the audited Felucca baseline.

See README.md and LICENSES/FELUCCA-LICENSING.txt for copyright and license.
Optionally supply FELUCCA_SOURCE_ZIP to compare against the upstream source.
"""
from pathlib import Path
import os,hashlib,json,sys,zipfile
r=Path(__file__).resolve().parents[1]
record=json.loads((r/'reference/felucca-hal-sha256.json').read_text())
ours=r/'firmware/fm1-native'
up=os.environ.get('FELUCCA_SOURCE_ZIP')
with zipfile.ZipFile(up) if up else open(os.devnull,'rb') as archive:
 for name,hash_expected in record['verified_critical_files'].items():
  actual=(ours/name).read_bytes()
  if hashlib.sha256(actual).hexdigest()!=hash_expected:
   sys.exit('FAIL: critical HAL/bootstrap baseline differs: '+name)
  if up:
   original=archive.read('Felucca-main/firmware/'+('src/' if name=='lcd.c' else '')+name)
   if original!=actual:sys.exit('FAIL: source ZIP differs: '+name)
print('PASS',len(record['verified_critical_files']),'audited upstream HAL/boot hashes verified; USB/OTA excluded from parity claims')
