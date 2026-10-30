#!/usr/bin/env python3
"""Embed V1.85 release bytes into the retained WebInstaller; SHA-256 checked."""
import re,base64,hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'RELEASE-MANIFEST.json').read_text())
assert manifest['release_version']=='1.85' and manifest['firmware_identity']=='FM-1_1500'
p=root/'WebInstaller.html';html=p.read_text().replace('V1.84','V1.85')
html=html.replace('V1.85 WAVE AMP + AUDIO-RATE SHAPING','V1.85 DEEP DRAW + SMOOTH AUDIO TRANSITIONS')
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
for key,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 embedded=base64.b64decode(re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',html).group(1))
 assert embedded==(root/manifest['packages'][key]['file']).read_bytes()
p.write_text(html)
print('PASS V1.85 installer, firmware identity and both embedded package hashes')
