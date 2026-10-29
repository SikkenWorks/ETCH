#!/usr/bin/env python3
"""V1.76 packaging, checksum, identity and native-source preservation gates.
Software only, never proof of physical FM1 OTA or safe flashing.
"""
import base64,hashlib,json,re
from pathlib import Path
import fm1_fwsc as fw
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'RELEASE-MANIFEST.json').read_text())
assert manifest['release_version']=='1.76'
assert manifest['firmware_identity']=='FM-1_1500'
assert manifest['project_format']=='PJT31'
assert manifest['hardware_verified'] is False
assert manifest['primary_fwsc']=='release/ETCH-V1.76.fwsc'
original=root/'release/ETCH-V1.75.fwsc'
new=root/'release/ETCH-V1.76.fwsc'
assert original.is_file() and new.is_file()
for f in (new,root/'compat/ETCH-V1.76-LEGACY-STOCK-HEAD.fwsc'):
 report,errors=fw.verify_fwsc(f)
 assert not errors,errors
 assert report['product']=='FM-1_1500'
prev=fw.parse_ufw(original.read_bytes())
current=fw.parse_ufw(new.read_bytes())
def entry(c,n):return next(x['data'] for x in c['entries'] if x['name']==n)
assert entry(prev,'flash.bin')[:0x4000]==entry(current,'flash.bin')[:0x4000], 'boot-header changed'
assert entry(prev,'ota.bin')==entry(current,'ota.bin'), 'trusted OTA loader changed'
assert entry(prev,'flash.bin')[0x4000:]!=entry(current,'flash.bin')[0x4000:], 'no application change'
html=(root/'WebInstaller.html').read_text()
for kind,scriptid in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 match=re.search(r'<script id="'+scriptid+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',html)
 assert match,scriptid
 raw=base64.b64decode(match.group(1))
 f=root/manifest['packages'][kind]['file']
 assert f.read_bytes()==raw,kind
 digest=hashlib.sha256(raw).hexdigest()
 assert manifest['packages'][kind]['sha256']==digest,kind
 assert digest in html,kind
assert "ID_ET: 'FM-1_1500'" in html
assert 'V176' in html and 'V1.76' in html
assert 'FELUCCA_ID "FM-1_1500"' in (root/'firmware/fm1-native/bringup.c').read_text()
pres=json.loads((root/'release/V1.76-SOURCE-PRESERVATION.json').read_text())
assert not pres['removed']
assert len(pres['modified_native_sources'])==3
m=json.loads((root/'release/V1.76-SRAM-AUDIT.json').read_text())
assert m['areas']['.pool']['remaining_bytes']>=12292
print('PASS V1.76 full package CRC, original boot/loader, identity, installer SHA, native-source preservation, 12KB RAM headroom')
