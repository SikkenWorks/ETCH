#!/usr/bin/env python3
"""Read-only native OTA CHECK/replay for ORIGINAL third-party .fwsc packages.

Example:
  python tools/replay_external_fm1_ota.py --package ./felucca-1.6.fwsc --version felucca-1.6

This tool compiles/executes the *real ETCH V1.50 ota.c* inside a fake NOR and
synthetic SysEx host. It never sends MIDI, accesses USB, or writes hardware.
"""
from pathlib import Path
import argparse, subprocess, hashlib, tempfile, sys

PINS = {
    'felucca-1.6': 'b2c1317b89d8d701c832627ae7e916785660e32a88422d57417293c8ec8679f8',
    'felucca-1.6.1': 'dff3a38f7e5b9833e15aba322558c9485e9e422e3ff813c616c91b3584a2d909',
}
root=Path(__file__).resolve().parents[1]
pa=argparse.ArgumentParser(description=__doc__)
pa.add_argument('--package',required=True,type=Path)
pa.add_argument('--version', choices=['unverified',*PINS],default='unverified')
a=pa.parse_args()
blob=a.package.read_bytes()
if a.package.suffix.lower()!='.fwsc' or not 500000 <= len(blob) <= 1500000:
    sys.exit('BLOCK: expected a standard, original FM1 .fwsc file')
digest=hashlib.sha256(blob).hexdigest()
print(f'Package: {a.package.name} | {len(blob)} bytes | SHA-256 {digest}',flush=True)
if a.version in PINS and digest!=PINS[a.version]:
    sys.exit('BLOCK: package SHA-256 does not match the published official release')
with tempfile.TemporaryDirectory(prefix='etch-v150-ota-') as temp:
    dst=Path(temp)/'ota_replay'
    subprocess.run(['cc','-std=c11','-O2', '-I',str(root/'firmware/fm1-native'),
                    str(root/'tests/test_v150_native_ota.c'),
                    str(root/'firmware/fm1-native/native_digest.c'), '-o',str(dst)],check=True)
    reference=root/'compat/V15-VERIFIED-BOOTHEAD.bin'
    subprocess.run([str(dst),str(a.package),str(reference)],check=True,timeout=80)
print('PASS: native CHECK succeeded with known-good boothead and trusted loader in simulated NOR.',flush=True)
print('HARDWARE UNVERIFIED: this test does NOT flash or demonstrate boot/update success on an FM1.',flush=True)
