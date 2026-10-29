#!/usr/bin/env python3
"""Software-only preservation and packaging checks for V1.77."""
import base64,hashlib,json,re
from pathlib import Path
import fm1_fwsc as fw
r=Path(__file__).resolve().parents[1]
m=json.loads((r/'RELEASE-MANIFEST.json').read_text())
assert m['firmware_identity']=='FM-1_1500' and m['release_version']=='1.77' and not m['hardware_verified']
assert m['project_format']=='PJT31' and m['primary_fwsc']=='release/ETCH-V1.77.fwsc'
old=fw.parse_ufw((r/'release/ETCH-V1.76.fwsc').read_bytes())
new=fw.parse_ufw((r/'release/ETCH-V1.77.fwsc').read_bytes())
def ent(x,name):return next(e['data'] for e in x['entries'] if e['name']==name)
assert ent(old,'flash.bin')[:0x4000]==ent(new,'flash.bin')[:0x4000], 'bootheader must remain unchanged'
assert ent(old,'ota.bin')==ent(new,'ota.bin'), 'trusted OTA loader must remain unchanged'
assert ent(old,'flash.bin')[0x4000:]!=ent(new,'flash.bin')[0x4000:], 'app must change'
installer=(r/'WebInstaller.html').read_text()
for kind,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 x=re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',installer)
 assert x,tag
 embedded=base64.b64decode(x.group(1))
 f=r/m['packages'][kind]['file'];assert f.read_bytes()==embedded,kind
 assert hashlib.sha256(embedded).hexdigest()==m['packages'][kind]['sha256']
 assert fw.verify_fwsc(f)[1]==[],kind
 assert fw.verify_fwsc(f)[0]['product']=='FM-1_1500'
assert '#define FELUCCA_ID "FM-1_1500"' in (r/'firmware/fm1-native/bringup.c').read_text()
assert '#define ETCH_UI_VERSION "V177"' in (r/'firmware/fm1-native/bringup.c').read_text()
assert 'V1.77' in installer
assert (r/'ETCH-Project-Preset-Manager.html').exists()
print('PASS V1.77 preserved exact loader, 16KiB boot, identity FM-1_1500, firmware/installer SHA-256, native version')
