#!/usr/bin/env python3
"""V1.75 release identity, container CRC, installer parity and preserved native assets.
Host-only and read-only. Does not establish physical FM1 OTA safety.
"""
import base64, hashlib, json, re
from pathlib import Path
import fm1_fwsc as fw
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'RELEASE-MANIFEST.json').read_text())
assert manifest['release_version']=='1.75'
assert manifest['firmware_identity']=='FM-1_1500'
path=root/'release/ETCH-V1.75.fwsc'
report, errors=fw.verify_fwsc(path)
assert not errors,errors
assert report['product']=='FM-1_1500'
original=root/'release/ETCH-V1.74.fwsc'
prev=fw.parse_ufw(original.read_bytes())
new=fw.parse_ufw(path.read_bytes())
def entry(parsed,name):
 return next(e['data'] for e in parsed['entries'] if e['name']==name)
assert entry(prev,'flash.bin')[:0x4000]==entry(new,'flash.bin')[:0x4000],'boot header drift'
assert entry(prev,'ota.bin')==entry(new,'ota.bin'),'trusted updater drift'
html=(root/'WebInstaller.html').read_text()
for kind,scriptid in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 match=re.search(r'<script id="'+scriptid+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',html)
 assert match,scriptid
 raw=base64.b64decode(match.group(1))
 rel=root/manifest['packages'][kind]['file']
 assert rel.read_bytes()==raw,kind
 digest=hashlib.sha256(raw).hexdigest()
 assert manifest['packages'][kind]['sha256']==digest,kind
 assert digest in html,kind
 assert fw.verify_fwsc(rel)[1]==[],kind
assert 'ID_ET: \'FM-1_1500\'' in html
assert 'V175' in html and 'V1.75' in html
assert 'FELUCCA_ID "FM-1_1500"' in (root/'firmware/fm1-native/bringup.c').read_text()
assert not json.loads((root/'release/V1.75-SOURCE-PRESERVATION.json').read_text())['removed']
m=json.loads((root/'release/V1.75-SRAM-AUDIT.json').read_text())
assert m['bytes_recovered_vs_v174']==12288
assert m['areas']['.pool']['remaining_bytes']==12292
print('PASS V1.75: UFW integrity, original boot header and OTA loader, unchanged FM-1_1500, both embedded firmware hashes, 12,288 RAM saved')
