#!/usr/bin/env python3
"""ETCH V1.88 installer: version text and BOTH embedded fwsc payloads verified."""
import re,base64,hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'RELEASE-MANIFEST.json').read_text())
assert manifest['release_version']=='1.88' and manifest['firmware_identity']=='FM-1_1500'
p=root/'WebInstaller.html'
html=p.read_text()
# Change ONLY literal page text and JS messages; do not touch packaged firmware until verified.
html=html.replace('V1.87','V1.88').replace('V187','V188')
html=html.replace('Adaptive DRAW · Audio Priority','Waveform Engine 3.0')
html=html.replace('Adaptive DRAW / Audio Priority','Waveform Engine 3.0')
html=html.replace('128-point editing, DETAIL / SOFT / EDGE brushes, DRAW keyboard isolation and smoother audio transitions.',
 'Waveform Engine 3.0: PHASE WARP and HALF SYMM tools with non-destructive preview, 128-point DRAW and responsive, load-adaptive audio publishing.')
for key,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 raw=(root/manifest['packages'][key]['file']).read_bytes()
 assert hashlib.sha256(raw).hexdigest()==manifest['packages'][key]['sha256']
 pattern=r'(<script id="'+tag+r'" type="application/octet-stream">)\s*[^<]*?\s*(</script>)'
 html,n=re.subn(pattern,lambda m:m.group(1)+'\n'+base64.b64encode(raw).decode()+'\n'+m.group(2),html)
 assert n==1,tag
for field,key in [('HASH_ET','etch'),('HASH_ET_STOCKHEAD','stock')]:
 pattern=field+r": '[0-9a-f]{64}'"
 html,n=re.subn(pattern,field+": '"+manifest['packages'][key]['sha256']+"'",html)
 assert n==1,field
assert "ID_ET: 'FM-1_1500'" in html
assert 'V1.87' not in html and 'V187' not in html and 'V181' not in html
assert 'V1.88' in html and 'V188' in html
for key,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 embedded=base64.b64decode(re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',html).group(1))
 assert embedded==(root/manifest['packages'][key]['file']).read_bytes()
p.write_text(html)
print('PASS V1.88 WebInstaller: version text, identity, embedded primary/legacy binaries and SHA-256')
