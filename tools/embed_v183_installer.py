#!/usr/bin/env python3
"""Embed V1.83 packages exactly, preserving the existing installer logic."""
import base64,hashlib,json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'RELEASE-MANIFEST.json').read_text())
assert manifest['firmware_identity']=='FM-1_1500' and manifest['release_version']=='1.83'
p=root/'WebInstaller.html';s=p.read_text()
s=s.replace('V1.82','V1.83')
s=s.replace('V1.83 SAFE STORAGE SETUP + PRESET BACKUP','V1.83 UI ALIGNMENT + PHOSPHOR')
s=s.replace('V1.83: PHOSPHOR color redesign across native FM1 screens.',
'''V1.83: header S/A workflow indicator aligned after the version on every versioned UI page; four-column parameter labels and values centered. The MORPH editor uses non-overlapping label/value pairs. PHOSPHOR colors remain unchanged.''')
for key,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 raw=(root/manifest['packages'][key]['file']).read_bytes()
 assert hashlib.sha256(raw).hexdigest()==manifest['packages'][key]['sha256']
 pat=r'(<script id="'+tag+r'" type="application/octet-stream">)\s*[^<]*?\s*(</script>)'
 s,n=re.subn(pat,lambda m:m.group(1)+'\n'+base64.b64encode(raw).decode()+'\n'+m.group(2),s)
 assert n==1,tag
for field,key in [('HASH_ET','etch'),('HASH_ET_STOCKHEAD','stock')]:
 pat=field+r": '[0-9a-f]{64}'"
 s,n=re.subn(pat,field+": '"+manifest['packages'][key]['sha256']+"'",s)
 assert n==1,field
assert "ID_ET: 'FM-1_1500'" in s
for key,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 raw=base64.b64decode(re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',s).group(1))
 assert hashlib.sha256(raw).hexdigest()==manifest['packages'][key]['sha256']
p.write_text(s)
print('PASS V1.83 installer embedded firmware/SHA-256 match both package variants and identity')
