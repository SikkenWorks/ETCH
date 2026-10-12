#!/usr/bin/env python3
"""Generate ETCH V0.95 single-file webinstaller from verified V0.94 template.
No device access. Pins exact package hash and V0.89-compatible flash header.
"""
from pathlib import Path
import base64, hashlib, re
root=Path(__file__).resolve().parents[1]
rel=root.parent
template=rel/'reference/ETCH-WebInstaller-V0.94.html'
package=rel/'ETCH-V0.95-V089-BOOT-COMPAT-EXPERIMENTAL.fwsc.lab'
# Source is pinned to the exact pre-fix release. No accidental text/format drift.
s=template.read_text()
assert 'FM-1_994' in s and 'V0.94' in s
old_hash='86ebaa698ef03cd6d1141ede18d4c745693e5db9ce002c664e66057adf7e101f'
assert old_hash in s
new_hash=hashlib.sha256(package.read_bytes()).hexdigest()
s=s.replace('V0.94','V0.95').replace('FM-1_994','FM-1_995').replace(old_hash,new_hash)
s=s.replace('for FM-1_989 ONLY','for FM-1_989 / FM-1_994 ONLY')
s=s.replace('V0.89 OTA compatibility + BPM + cutoff','V0.89 boot compatibility + PROJECTS + BPM + cutoff')
s=s.replace('Experimental V0.95 for FM-1_989 / FM-1_994 ONLY. Original V0.89 boot header preserved; BPM persistence and cutoff steps of 10.',
  'Experimental V0.95 upgrade from ETCH V0.89 or V0.94. Original compatible boot header preserved; 4 project slots enabled, BPM saved, CUTOFF step 10.')
original="if(choice==='etch' && device.id.text!=='FM-1_989')"
replacement="if(choice==='etch' && !['FM-1_989','FM-1_994'].includes(device.id.text))"
assert s.count(original)==1
s=s.replace(original,replacement)
s=s.replace('This V0.95 compatibility build supports only an FM1 currently running ETCH V0.89 (FM-1_989). Detected ',
            'This V0.95 compatibility build supports ETCH V0.89 (FM-1_989) or V0.94 (FM-1_994). Detected ')
s=s.replace('ETCH V0.95 LAB OTA diagnostics + Sequencer', 'ETCH V0.95 LAB OTA diagnostics + Projects')
assert "['FM-1_989','FM-1_994'].includes(device.id.text)" in s
assert s.count('KNOWN_HEAD_HASH')>=2
# Base64 embedded firmware byte-for-byte identical to the verified package.
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)([\s\S]*?)(</script>)'
assert len(re.findall(pat,s))==1
enc=base64.b64encode(package.read_bytes()).decode('ascii')
enc='\n'.join(enc[i:i+144] for i in range(0,len(enc),144))
s=re.sub(pat,lambda m:m.group(1)+'\n'+enc+'\n'+m.group(3),s,count=1)
assert 'FM-1_994' in s and 'FM-1_995' in s and 'V0.95' in s
out=rel/'ETCH-WebInstaller-V0.95.html';out.write_text(s)
print('PASS V0.95 html',out.stat().st_size,'embedded fwsc sha256',new_hash)
