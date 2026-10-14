#!/usr/bin/env python3
"""Prove embedded V1.28 container carries same FM1 app as release APP.bin.
Only reads local files; NEVER sends MIDI, never writes to hardware.
"""
import sys,base64,re,hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as fw
html=(root/'ETCH-WebInstaller-V1.28.html').read_text()
encoded=re.search(r'<script id="firmware-embedded" type="application/octet-stream">([\s\S]*?)</script>',html)
assert encoded,'Embedded package missing'
raw=base64.b64decode(''.join(encoded.group(1).split()),validate=True)
packet=fw.parse_ufw(raw)
assert packet['product']=='FM-1_1028'
flash=next(e['data'] for e in packet['entries'] if e['name']=='flash.bin')
section=bytearray(flash[fw.APP_AREA_FLASH:])
fw.sfc_xor(section,0,len(section),0,fw.DEFAULT_CHIP_KEY)
expected=(root/'ETCH-V1.28-APP.bin').read_bytes()
actual=bytes(section[0x120:0x120+len(expected)])
assert actual==expected,'Installer contains an app different from ETCH-V1.28-APP.bin'
assert set(section[0x120+len(expected):0x120+fw.APP_SLOT])=={255},'App area unexpected padding'
manifest=json.loads((root/'RELEASE-MANIFEST.json').read_text())
sha=lambda b: hashlib.sha256(b).hexdigest()
assert manifest['app_sha256']==sha(expected) and manifest['app_bytes']==len(expected)
assert manifest['experimental_firmware_sha256']==sha(raw)
assert manifest['webinstaller_sha256']==sha(html.encode())
assert manifest['firmware_identity']==packet['product']
print('PASS V1.28 app.bin is byte-identical to decrypted installer application')
print('PASS app/container/html hashes match manifest; firmware identity FM-1_1028')
