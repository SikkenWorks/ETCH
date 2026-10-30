#!/usr/bin/env python3
"""Embed the two V1.82 variants with matching hashes, retain original FM-1_1500 identity."""
import base64,hashlib,json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'RELEASE-MANIFEST.json').read_text()); assert manifest['firmware_identity']=='FM-1_1500'
p=root/'WebInstaller.html';s=p.read_text()
s=s.replace('V1.81','V1.82').replace('V180','V182')
s=s.replace('ADVANCED FX BYPASS + DSP GATING','PHOSPHOR COLORS + FLAT WAVE EDIT')
s=s.replace('V1.82 introduces a RAM-only preset snapshot protocol and save feedback.',
'''V1.82 adds the ADVANCED long-press FX BYPASS manager. Long press FX for 700ms, turn K1 to scroll, use K4/SEL to toggle a configured effect, K2 to bypass all and K3 to restore. The runtime bypass leaves PJT31 and saved effect parameters unchanged. Expensive time-domain algorithms skip work for bypassed chorus, delay and reverb. The previous boot visual, safe storage prompts, preset manager, USB audio and OTA identity remain unchanged. Software tests only; hardware safety and actual FM1 CPU savings remain unverified.''')
s=s.replace('V1.82: RAM-only sound backup via USB-MIDI, .etchpreset import/export and SAVE / VERIFYING status.',
'''V1.82: long FX hold in ADVANCED opens an effect-wide bypass screen (22 FX positions). Global effects, Wave A/B inserts and Wave C modulation can be bypassed without altering saved settings. Storage recovery, USB audio and OTA identity are preserved.''')
s=s.replace('V1.81: long FX hold in ADVANCED opens an effect-wide bypass screen (22 FX positions). Global effects, Wave A/B inserts and Wave C modulation can be bypassed without altering saved settings. Storage recovery, USB audio and OTA identity are preserved.', 'V1.82: PHOSPHOR color redesign across native FM1 screens. In DRAW, long press EDIT to choose a flat zero waveform starting line (YES/NO, default NO); YES is undoable. Existing FX BYPASS, projects, USB audio and OTA identity remain preserved.')
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
s=s.replace('V1.81: long FX hold in ADVANCED opens an effect-wide bypass screen (22 FX positions). Global effects, Wave A/B inserts and Wave C modulation can be bypassed without altering saved settings. Storage recovery, USB audio and OTA identity are preserved.', 'V1.82: PHOSPHOR color redesign across native FM1 screens. In DRAW, long press EDIT to choose a flat zero waveform starting line (YES/NO, default NO); YES is undoable. Existing FX BYPASS, projects, USB audio and OTA identity remain preserved.')
for key,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 raw=base64.b64decode(re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',s).group(1))
 assert hashlib.sha256(raw).hexdigest()==manifest['packages'][key]['sha256']
p.write_text(s)
print('PASS V1.82 installer embed, both packages SHA256, product FM-1_1500')
