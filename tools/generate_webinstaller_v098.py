#!/usr/bin/env python3
"""Regenerate V0.98 webinstaller offline from the included previous installer and binary."""
from pathlib import Path
import hashlib,base64,re
root=Path(__file__).resolve().parents[2]
fw=root/'ETCH-V0.98-V089-BOOT-COMPAT-EXPERIMENTAL.fwsc.lab'
old=root/'reference/ETCH-WebInstaller-V0.97.html'
oldhash='a2304456c215cc55b44a43dd61e236b2def2abc9c4ee36dceb2ca0341ce470cb'
newhash=hashlib.sha256(fw.read_bytes()).hexdigest()
s=old.read_text()
assert s.count(oldhash)==1
s=s.replace('V0.97','V0.98').replace('FM-1_997','FM-1_998').replace(oldhash,newhash)
s=s.replace("['FM-1_989','FM-1_994','FM-1_995','FM-1_996'].includes(device.id.text)","['FM-1_989','FM-1_994','FM-1_995','FM-1_996','FM-1_997'].includes(device.id.text)")
s=s.replace('FM-1_989 / FM-1_994 / FM-1_995 / FM-1_996 ONLY','FM-1_989 / FM-1_994 / FM-1_995 / FM-1_996 / FM-1_997 ONLY')
s=s.replace('V0.89, V0.94, V0.95 or V0.96','V0.89, V0.94, V0.95, V0.96 or V0.97')
s=s.replace('V0.95 (FM-1_995) or V0.96 (FM-1_996)','V0.95 (FM-1_995), V0.96 (FM-1_996) or V0.97 (FM-1_997)')
s=s.replace('MONO/POLY + STARTUP AUDIO + PROJECTS + BPM','SCALE GUIDE KEYBOARD LEDS + MONO/POLY + PROJECTS + BPM')
s=s.replace('MONO/POLY via OCT- and OCT+, stored per project; boot-audio guard, 4 projects, BPM, cutoff step 10.','SCALE GUIDE lights all notes in selected scale across the keyboard; MONO/POLY, project save, startup audio guard and cutoff step 10 preserved.')
s=s.replace('MONO/POLY and startup audio guard','all-scale GUIDE keyboard LEDs and existing startup audio guard')
s=s.replace('V0.89, V0.94, V0.95 or V0.96','V0.89, V0.94, V0.95, V0.96 or V0.97')
pattern=r'(<script id="firmware-embedded" type="application/octet-stream">)([\s\S]*?)(</script>)'
assert len(re.findall(pattern,s))==1
e=base64.b64encode(fw.read_bytes()).decode('ascii');e='\n'.join(e[i:i+144] for i in range(0,len(e),144))
s=re.sub(pattern,lambda m:m.group(1)+'\n'+e+'\n'+m.group(3),s,count=1)
s=s.replace('ET_LENGTH: 610086',f'ET_LENGTH: {fw.stat().st_size}')
(root/'ETCH-WebInstaller-V0.98.html').write_text(s)
print('Regenerated ETCH V0.98 installer',hashlib.sha256(s.encode()).hexdigest())
