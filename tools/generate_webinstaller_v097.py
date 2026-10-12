#!/usr/bin/env python3
"""Regenerate ETCH V0.97 installer from the bundled V0.96 HTML and exact V0.97 firmware."""
from pathlib import Path
import re,hashlib,base64
root=Path(__file__).resolve().parents[2]
old=root/'reference/ETCH-WebInstaller-V0.96.html'
fw=root/'ETCH-V0.97-V089-BOOT-COMPAT-EXPERIMENTAL.fwsc.lab'
s=old.read_text()
old_fw_sha='e2e92922604751ebe785d1fc51e6b3195c519972516b90591f5a33aa6bda1300'
new_fw_sha=hashlib.sha256(fw.read_bytes()).hexdigest()
assert s.count(old_fw_sha)==1
s=s.replace('V0.96','V0.97').replace('FM-1_996','FM-1_997').replace(old_fw_sha,new_fw_sha)
s=s.replace("['FM-1_989','FM-1_994','FM-1_995'].includes(device.id.text)","['FM-1_989','FM-1_994','FM-1_995','FM-1_996'].includes(device.id.text)")
s=s.replace('FM-1_989 / FM-1_994 / FM-1_995 ONLY','FM-1_989 / FM-1_994 / FM-1_995 / FM-1_996 ONLY')
s=s.replace('or V0.95', 'or V0.96').replace('or V0.96 (FM-1_995)', 'V0.95 (FM-1_995) or V0.96 (FM-1_996)')
s=s.replace('from ETCH V0.89, V0.94 or V0.96','from ETCH V0.89, V0.94, V0.95 or V0.96')
s=s.replace('This V0.97 compatibility build supports ETCH V0.89 (FM-1_989), V0.94 (FM-1_994) or V0.96 (FM-1_995).','This V0.97 compatibility build supports ETCH V0.89 (FM-1_989), V0.94 (FM-1_994), V0.95 (FM-1_995) or V0.96 (FM-1_996).')
s=s.replace('RING MOD + CHANNEL EQ + PROJECTS + BPM','MONO/POLY + STARTUP AUDIO + PROJECTS + BPM')
s=s.replace('Ring Mod and 3-band Channel EQ + HPF, 4 project slots enabled, BPM saved, CUTOFF step 10.','MONO/POLY via OCT- and OCT+, stored per project; boot-audio guard, 4 projects, BPM, cutoff step 10.')
s=s.replace('V0.89 boot-head compatibility, Ring Mod, Channel EQ and final ACK grace','V0.89 boot-head compatibility, MONO/POLY and startup audio guard')
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)([\s\S]*?)(</script>)'
assert len(re.findall(pat,s))==1
e=base64.b64encode(fw.read_bytes()).decode();e='\n'.join(e[i:i+144] for i in range(0,len(e),144))
s=re.sub(pat,lambda m:m.group(1)+'\n'+e+'\n'+m.group(3),s,count=1)
s=s.replace('ET_LENGTH: 610086',f'ET_LENGTH: {fw.stat().st_size}')
(root/'ETCH-WebInstaller-V0.97.html').write_text(s)
print('OK',hashlib.sha256(s.encode()).hexdigest())
