#!/usr/bin/env python3
"""Offline only: embeds a statically checked experimental candidate, not a certified release."""
import base64,hashlib,re
from pathlib import Path
import fm1_fwsc as fw
root=Path(__file__).resolve().parents[1]
old=root/'ETCH-WebInstaller-V1.31.html'; target=root/'ETCH-WebInstaller-V1.32.html'
package=root/'internal-v132-candidate.fwsc.lab'
report,errors=fw.verify_fwsc(package)
if errors or report['product']!='FM-1_1032': raise ValueError('V1.32 candidate not verified')
data=package.read_bytes(); h=hashlib.sha256(data).hexdigest()
s=old.read_text()
oldhash=re.search(r"HASH_ET: '([a-f0-9]{64})'",s).group(1)
s=s.replace(oldhash,h).replace('FM-1_1031','FM-1_1032')
# Project format stays PJT27; this is a realtime DSP-only maintenance update.
s=s.replace('Per-preset ORGANIC, LIVING and MOD MATRIX','Per-preset ORGANIC, LIVING and MOD MATRIX')
s=s.replace('LIVING, MOD MATRIX and SPECTRAL', 'LIVING, MOD MATRIX and SPECTRAL')
# Explicitly disclose deferred sound changes instead of disguising latency.
s=s.replace('Includes A/B waveform drawing', 'Includes V1.32 polyphony deadline optimizations, SPECTRAL controls, LIVING, PATCH BLEND, and A/B waveform drawing')
s=s.replace('V1.31', 'V1.32')
# Include exactly this candidate, independent of any old embedded payload.
pat=r'(<script id="firmware-embedded" type="application/octet-stream">)([\s\S]*?)(</script>)'
assert len(re.findall(pat,s))==1
encoded=base64.b64encode(data).decode('ascii'); encoded='\n'.join(encoded[i:i+144] for i in range(0,len(encoded),144))
s=re.sub(pat,lambda m:m.group(1)+'\n'+encoded+'\n'+m.group(3),s,count=1)
assert s.count(h)==1
assert "ID_ET: 'FM-1_1032'" in s
assert "ET_LENGTH: 610086" in s
assert s.count('https://github.com/SikkenWorks/ETCH')>=1
target.write_text(s)
print('Wrote',target.name,len(s),'bytes; embedded package SHA256',h)
