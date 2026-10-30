#!/usr/bin/env python3
"""V1.88 firmware/installer payload consistency, read-only host audit."""
from pathlib import Path
import sys,base64,json,re,hashlib
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as fw
m=json.loads((root/'RELEASE-MANIFEST.json').read_text())
assert m['release_version']=='1.88' and m['firmware_identity']=='FM-1_1500' and m['project_format']=='PJT32'
html=(root/'WebInstaller.html').read_text()
assert "ID_ET: 'FM-1_1500'" in html
for kind,older in [('etch','release/ETCH-V1.87.fwsc'),('stock','compat/ETCH-V1.87-LEGACY-STOCK-HEAD.fwsc')]:
 filename=m['packages'][kind]['file'];raw=(root/filename).read_bytes();prev=(root/older).read_bytes()
 result,errors=fw.verify_fwsc(root/filename)
 assert not errors and result['product']=='FM-1_1500'
 digest=hashlib.sha256(raw).hexdigest()
 assert digest==m['packages'][kind]['sha256']
 new_entries=fw.parse_ufw(raw)['entries'];old_entries=fw.parse_ufw(prev)['entries']
 for name in ('flash.bin','ota.bin'):
  x=next(e['data'] for e in new_entries if e['name']==name)
  y=next(e['data'] for e in old_entries if e['name']==name)
  if name=='ota.bin':assert x==y
  else:assert x[:0x4000]==y[:0x4000]
 tag='firmware-embedded' if kind=='etch' else 'firmware-embedded-stockhead'
 q=re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',html)
 assert q and base64.b64decode(q[1])==raw
 assert digest in html
 print('PASS',kind,filename,digest)
assert m['app_sha256']==hashlib.sha256((root/'build/fm1-native-storage-updater-uac48-p05/app.bin').read_bytes()).hexdigest()
assert m['hardware_verified'] is False
print('PASS: V1.88 package, 16KiB bootloader, OTA loader, identity, dual installer hashes and no hardware claims')
