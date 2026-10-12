#!/usr/bin/env python3
"""Reproduce the ETCH V0.96 webinstaller using a pinned V0.95 template.

Offline only; does not communicate with an FM1. This keeps the version identity,
embedded bytes, protected header check and accepted source versions synchronized.
"""
from pathlib import Path
import re,base64,hashlib
root=Path(__file__).resolve().parents[2]
firmware=root/'ETCH-V0.96-V089-BOOT-COMPAT-EXPERIMENTAL.fwsc.lab'
template=root/'reference/ETCH-WebInstaller-V0.95.html'
old_hash='e91f8157e6af2372c180100175ab3e9b42c5e7c59db5b6d41eedd50cb6e39f51'
new_hash=hashlib.sha256(firmware.read_bytes()).hexdigest()
html=template.read_text()
assert old_hash in html
html=html.replace('V0.95','V0.96').replace('FM-1_995','FM-1_996').replace(old_hash,new_hash)
html=html.replace("['FM-1_989','FM-1_994'].includes(device.id.text)","['FM-1_989','FM-1_994','FM-1_995'].includes(device.id.text)")
html=html.replace('FM-1_989 / FM-1_994 ONLY','FM-1_989 / FM-1_994 / FM-1_995 ONLY')
html=html.replace('ETCH V0.89 (FM-1_989) or V0.94 (FM-1_994)', 'ETCH V0.89 (FM-1_989), V0.94 (FM-1_994) or V0.95 (FM-1_995)')
html=html.replace('ETCH V0.89 or V0.94','ETCH V0.89, V0.94 or V0.95')
html=html.replace('V0.89 boot compatibility + PROJECTS + BPM + cutoff', 'RING MOD + CHANNEL EQ + PROJECTS + BPM')
html=html.replace('4 project slots enabled, BPM saved, CUTOFF step 10.', 'Ring Mod and 3-band Channel EQ + HPF, 4 project slots enabled, BPM saved, CUTOFF step 10.')
html=html.replace('V0.89 boot-head compatibility, BPM persistence and final ACK grace','V0.89 boot-head compatibility, Ring Mod, Channel EQ and final ACK grace')
pattern=r'(<script id="firmware-embedded" type="application/octet-stream">)([\s\S]*?)(</script>)'
assert len(re.findall(pattern,html))==1
enc=base64.b64encode(firmware.read_bytes()).decode()
enc='\n'.join(enc[i:i+144] for i in range(0,len(enc),144))
html=re.sub(pattern,lambda m:m.group(1)+'\n'+enc+'\n'+m.group(3),html,count=1)
html=html.replace('ET_LENGTH: 610086',f'ET_LENGTH: {firmware.stat().st_size}')
out=root/'ETCH-WebInstaller-V0.96.html';out.write_text(html)
print('OK V0.96',new_hash,len(html))
