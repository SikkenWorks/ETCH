#!/usr/bin/env python3
"""Embed the two V1.78 variants with matching hashes, retain original FM-1_1500 identity."""
import base64,hashlib,json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'RELEASE-MANIFEST.json').read_text()); assert manifest['firmware_identity']=='FM-1_1500'
p=root/'WebInstaller.html';s=p.read_text()
s=s.replace('V1.77','V1.78').replace('V177','V178')
s=s.replace('PRESET BACKUP + SAVE FEEDBACK','SAFE STORAGE SETUP + PRESET BACKUP')
s=s.replace('V1.78 introduces a RAM-only preset snapshot protocol and save feedback.',
'''V1.78 adds an explicit NO-default first-boot storage question when the ETCH storage area contains incompatible data. YES requires a second SEL confirmation and a fresh three-second SAVE hold. Only the confirmed ETCH project sectors are erased; other FM1 sectors are protected. V1.77 RAM-only preset transfer and save feedback remain supported.''')
s=s.replace('V1.78: RAM-only sound backup via USB-MIDI, .etchpreset import/export and SAVE / VERIFYING status.',
'''V1.78: first-boot storage setup (NO default, SEL then 3-second SAVE hold for YES); permanent deletion of foreign sample slots 2+3 on explicit confirmation. RAM-only .etchpreset backup and SAVE / VERIFYING feedback remain available.''')
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
print('PASS V1.78 installer embed, both packages SHA256, product FM-1_1500')
