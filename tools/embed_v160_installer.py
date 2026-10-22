#!/usr/bin/env python3
"""Embed the two verified ETCH V1.60 packages in the existing WebInstaller.

The firmware identity is intentionally NOT versioned; remains FM-1_1500.
"""
from pathlib import Path
import base64, hashlib, json, re
ROOT=Path(__file__).resolve().parents[1]
h=ROOT/'WebInstaller.html'
s=h.read_text()
manifest=json.loads((ROOT/'RELEASE-MANIFEST.json').read_text())
if manifest['firmware_identity']!='FM-1_1500':raise ValueError('product identity drift')
for kind,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
    item=manifest['packages'][kind]
    raw=(ROOT/item['file']).read_bytes()
    digest=hashlib.sha256(raw).hexdigest()
    assert digest==item['sha256']
    pat=r'(<script id="'+tag+r'" type="application/octet-stream">)\s*[^<]*?\s*(</script>)'
    s,n=re.subn(pat,lambda m:m.group(1)+'\n'+base64.b64encode(raw).decode()+'\n'+m.group(2),s)
    assert n==1,(tag,n)
s,n=re.subn(r"HASH_ET: '[0-9a-f]{64}'", "HASH_ET: '"+manifest['packages']['etch']['sha256']+"'",s)
assert n==1
s,n=re.subn(r"HASH_ET_STOCKHEAD: '[0-9a-f]{64}'", "HASH_ET_STOCKHEAD: '"+manifest['packages']['stock']['sha256']+"'",s)
assert n==1
s=s.replace('V1.50', 'V1.60')
s=s.replace('V1.60 NEW FIRMWARE · B1 HF2','V1.60 CROSS-FIRMWARE OTA')
s=s.replace('NEW FM-1_1500 FIRMWARE','UNCHANGED ID FM-1_1500')
s=s.replace("this.progress('done',100,'Verified: '+pkg.product);this.log('Device confirmed firmware identity '+pkg.product+'.');",
'''const unchangedIdentity=initial.id.text===pkg.product;
    if(unchangedIdentity && pkg.kind.startsWith('etch')){
      this.progress('done',100,'OTA acknowledged; FM1 reconnected · confirm V160 on screen');
      this.log('The OTA loader acknowledged WRITE and FM1 reconnected as '+pkg.product+'. Identity is intentionally unchanged from V1.50: the SysEx handshake alone cannot attest V1.60. Verify V160 on the device.');
    }else{
      this.progress('done',100,'Verified firmware identity: '+pkg.product);
      this.log('OTA acknowledged, device restarted and reported '+pkg.product+'.');
    }''')
s=s.replace("'V1.60: uses the proven B1 HF2 Felucca/SLOOP two-stage loader transition from FM-1_900 and accepts authenticated ota-FM-1_9xx / ota-FM-1_015 loader identities. New native firmware build FM-1_1500; synthesis behavior unchanged. Physical ETCH installation remains unverified.'",
"'V1.60: native cross-firmware CRC + versioned-loader validation, trusted app-only staging, and unchanged FM-1_1500 package identity. Verify V160 on the device after installation. Real FM1 cross-firmware transitions have not been tested.'")
h.write_text(s)
assert "ID_ET: 'FM-1_1500'" in s
for kind,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
    raw=base64.b64decode(re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',s).group(1))
    assert hashlib.sha256(raw).hexdigest()==manifest['packages'][kind]['sha256']
print('PASS embedded V1.60 ETCH/stock-head .fwsc packages, matching SHA-256, preserved FM-1_1500')
