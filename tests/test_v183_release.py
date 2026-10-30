#!/usr/bin/env python3
"""Check V1.83 release against V1.82, including OTA handoff safety."""
from pathlib import Path
import sys,json,base64,re,hashlib
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as fw
H=lambda x:hashlib.sha256(x).hexdigest()
m=json.loads((root/'RELEASE-MANIFEST.json').read_text())
assert m['release_version']=='1.83'
assert m['firmware_identity']=='FM-1_1500'
assert m['project_format']=='PJT31'
html=(root/'WebInstaller.html').read_text()
assert "ID_ET: 'FM-1_1500'" in html
for key,ref in [('etch','release/ETCH-V1.82.fwsc'),('stock','compat/ETCH-V1.82-LEGACY-STOCK-HEAD.fwsc')]:
 file=root/m['packages'][key]['file'];raw=file.read_bytes();old=(root/ref).read_bytes()
 desc,errors=fw.verify_fwsc(file)
 assert not errors,(key,errors)
 assert desc['product']=='FM-1_1500'
 assert H(raw)==m['packages'][key]['sha256']
 cur=fw.parse_ufw(raw)['entries'];prev=fw.parse_ufw(old)['entries']
 current_flash=next(x['data'] for x in cur if x['name']=='flash.bin')
 previous_flash=next(x['data'] for x in prev if x['name']=='flash.bin')
 current_loader=next(x['data'] for x in cur if x['name']=='ota.bin')
 previous_loader=next(x['data'] for x in prev if x['name']=='ota.bin')
 assert current_flash[:16384]==previous_flash[:16384], 'bootheader changed'
 assert current_loader==previous_loader, 'loader changed'
 tag='firmware-embedded' if key=='etch' else 'firmware-embedded-stockhead'
 match=re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',html)
 assert match,tag
 assert base64.b64decode(match[1])==raw,'installer payload mismatch'
 assert H(raw) in html,'installer hash mismatch'
 print('PASS',key,'package',len(raw),'bytes, SHA256=',H(raw),'bootloader/header unchanged')
assert m['app_sha256']==H((root/'build/fm1-native-storage-updater-uac48-p05/app.bin').read_bytes())
assert m['hardware_verified'] is False
print('PASS V1.83 release/installer exact package parity, unchanged FM-1_1500/PJT31, no hardware claim')
