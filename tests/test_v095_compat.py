#!/usr/bin/env python3
"""Offline V0.89 -> V0.95 compatibility regression, never sends MIDI or flashes."""
from pathlib import Path
import sys,hashlib
root=Path(__file__).resolve().parents[1]
release=root.parent
sys.path.insert(0,str(root/'tools'))
import fm1_fwsc as F
from fm1_v095_compat import make_compat
original=release/'reference/ETCH-V0.89-FULL-FM1-EXPERIMENTAL.fwsc'
app=root/'build/fm1-native-storage-updater-p05/app.bin'
product=release/'ETCH-V0.95-V089-BOOT-COMPAT-EXPERIMENTAL.fwsc.lab'
old=F.parse_ufw(original.read_bytes())
new=F.parse_ufw(product.read_bytes())
old_head=old['entries'][0]['data'][:0x4000]
new_head=new['entries'][0]['data'][:0x4000]
assert old_head==new_head, 'Header must match V0.89 for OTA CHECK'
assert hashlib.sha256(new_head).hexdigest()=='d9f43191777656de01c7bed4f9e9ba2e34e94832c5d2ed02c3fc7ab7a6d9bd67'
assert new['product']=='FM-1_995'
assert old['entries'][1]['data']==new['entries'][1]['data'], 'OTA loader must stay identical'
assert new['entries'][0]['size']==old['entries'][0]['size']==0x93000
report,errors=F.verify_fwsc(product)
assert not errors,(errors,report)
# Re-packing is reproducible from exactly the supplied V0.89 image, and refuses altered input.
import tempfile
with tempfile.TemporaryDirectory(prefix='etch-v094-tests-') as temp:
    out=Path(temp)/'verify.fwsc.lab'
    result=make_compat(app,original,out)
    assert out.read_bytes()==product.read_bytes()
    assert result['identical_to_reference_head']
    bad=Path(temp)/'altered-v089.fwsc'
    raw=bytearray(original.read_bytes());raw[1130]^=1;bad.write_bytes(raw)
    try:make_compat(app,bad,Path(temp)/'bad.fwsc.lab')
    except ValueError:pass
    else:raise AssertionError('Tampered reference not rejected')
print('PASS V0.95 pin V0.89 head, pin unchanged loader, CRC/size, reproducible pack, tamper refusal')
