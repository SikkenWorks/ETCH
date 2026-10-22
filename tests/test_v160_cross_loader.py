#!/usr/bin/env python3
"""Host-only native replay: valid alternate LZ4 container and damaged variants.

This generates SYNTHETIC packaging fixtures from ETCH release assets, not actual
Felucca release files. It intentionally makes no claim of hardware coverage.
"""
from pathlib import Path
import hashlib
import subprocess
import sys
import tempfile
import struct
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import fm1_fwsc as fw
from fm1_lz4 import decode_loader

BASE = ROOT/'release/ETCH-FM1-V1.50-ETCH-HEAD.fwsc'
INSTALLED_HEAD = ROOT/'compat/V15-VERIFIED-BOOTHEAD.bin'


def literal_block(raw: bytes) -> bytes:
    assert 1 <= len(raw) <= 1024
    extra = len(raw)-15
    b = bytearray([0xF0 if extra >= 0 else len(raw)<<4])
    if extra >= 0:
        while extra >= 255:
            b.append(255); extra -= 255
        b.append(extra)
    b += raw
    return bytes(b)


def alternate_lz4_ota(ota: bytes, unknown_family=False) -> bytes:
    old = fw.validate_ota(ota)
    assert old['inner_data_crc_ok'] and old['inner_load_ok']
    expanded = decode_loader(ota[64:], old['inner_payload_bytes'])
    inner=bytearray(ota[32:64])
    if unknown_family:
        expanded=expanded.replace(b'FELUCCA-LOADER-1',b'UNKNOWN-LOADER-1',1)
        struct.pack_into('<H',inner,2,fw.crc16(expanded))
        struct.pack_into('<H',inner,0,fw.crc16(inner[2:]))
    compressed = bytearray()
    for p in range(0,len(expanded),1024):
        chunk=expanded[p:p+1024]
        enc=literal_block(chunk)
        compressed += struct.pack('<II',len(enc),len(chunk))+enc
    body=bytes(inner)+compressed
    outer=old['outer']
    head=fw.jlf_entry(fw.crc16(body),32,len(body),outer['flags'],outer['reserved'],outer['index'],'usb_hid_ota.bin')
    result=head+body
    new=fw.validate_ota(result)
    assert new['inner_data_crc_ok'] and new['outer_data_crc_ok'] and new['inner_load_ok']
    assert len(result)!=len(ota)
    return result


def main():
    original=fw.parse_ufw(BASE.read_bytes())
    assert original['product']=='FM-1_1500'
    items={x['name']:x['data'] for x in original['entries']}
    with tempfile.TemporaryDirectory(prefix='etch-v160-cross-') as temp:
        temp=Path(temp)
        alt=temp/'etch-compatible-alternate-loader.fwsc'
        alt.write_bytes(fw.build_fwsc(items['flash.bin'],alternate_lz4_ota(items['ota.bin']), original['product']))
        report,errors=fw.verify_fwsc(alt)
        assert not errors,errors
        assert report['product']==original['product']
        assert report['ota']['bytes']!=len(items['ota.bin'])
        exe=temp/'ota_native'
        subprocess.run(['cc','-std=c11','-O2',str(ROOT/'tests/test_v150_native_ota.c'),str(ROOT/'firmware/fm1-native/native_digest.c'),'-o',str(exe)],check=True)
        good=subprocess.run([str(exe),str(alt),str(INSTALLED_HEAD)],capture_output=True,text=True)
        assert good.returncode==0,(good.stdout,good.stderr)
        assert 'TRUSTED_STAGED_LOADER=1' in good.stdout
        assert 'BOOT_HEAD_UNCHANGED=1' in good.stdout
        assert 'OTA_ID=ota-FM-1_015' in good.stdout
        print('PASS: original product identity FM-1_1500 retained')
        print('PASS: differently compiled/encoded compatible versioned loader accepted')
        print('PASS: incoming executable NOT staged (trusted binary staged instead)')
        print('PASS: protected boot region untouched, confirmed OTA identity')
        print('SIMULATED FIXTURE SHA256',hashlib.sha256(alt.read_bytes()).hexdigest())
        # Damage in raw package must be rejected before any simulated staging erase.
        damage=bytearray(alt.read_bytes());damage[-3]^=1
        broken=temp/'etch-corrupt.fwsc';broken.write_bytes(damage)
        bad=subprocess.run([str(exe),str(broken),str(INSTALLED_HEAD)],capture_output=True,text=True)
        assert bad.returncode!=0 and 'COMMITTED=0' in bad.stdout,(bad.stdout,bad.stderr)
        print('PASS: damaged incoming package refused with no commit')
        unknown=temp/'unknown-family.fwsc'
        unknown.write_bytes(fw.build_fwsc(items['flash.bin'],alternate_lz4_ota(items['ota.bin'],unknown_family=True),original['product']))
        report,errors=fw.verify_fwsc(unknown)
        assert not errors,errors  # container integrity itself is valid
        bad=subprocess.run([str(exe),str(unknown),str(INSTALLED_HEAD)],capture_output=True,text=True)
        assert bad.returncode!=0 and 'CROSS_OTA_RESULT=-16' in bad.stdout and 'COMMITTED=0' in bad.stdout,(bad.stdout,bad.stderr)
        print('PASS: correctly checksummed unknown loader family blocked before staging')
    print('LIMIT: No original external Felucca/stock package and no physical FM1 were used.')

if __name__=='__main__':main()
