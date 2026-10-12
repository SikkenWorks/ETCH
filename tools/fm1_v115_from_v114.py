#!/usr/bin/env python3
"""Reproducible ETCH 1.15 LAB package from a SHA-pinned ETCH 1.14 package.

Preserves exact protected flash header and OTA loader. Offline verifier, not a
hardware/recovery certification. Derived ETCH firmware source is GPL-3.0-only.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import fm1_fwsc as fw

PINNED_V114 = 'e63bf148295535c5146fae64113344f7f154afb5e28bc0e332e81fb496f661a0'
PINNED_HEAD = 'd9f43191777656de01c7bed4f9e9ba2e34e94832c5d2ed02c3fc7ab7a6d9bd67'
HEAD_BYTES = 16384


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def build(app_file: Path, reference_file: Path, output: Path) -> dict:
    if not output.name.endswith('.fwsc.lab'):
        raise ValueError('Only explicitly experimental .fwsc.lab output is allowed')
    if sha(reference_file.read_bytes()) != PINNED_V114:
        raise ValueError('V1.14 reference SHA256 mismatch. Refusing incompatible header')
    reference = fw.parse_ufw(reference_file.read_bytes())
    if reference['product'] != 'FM-1_1014':
        raise ValueError('Incorrect reference firmware identity')
    _, errors = fw.verify_fwsc(reference_file)
    if errors:
        raise ValueError('Invalid reference: ' + str(errors))
    assets = fw.extract_stock_assets(reference_file)
    old_flash = next(e['data'] for e in reference['entries'] if e['name'] == 'flash.bin')
    if sha(old_flash[:HEAD_BYTES]) != PINNED_HEAD:
        raise ValueError('Protected header is not approved 0.89-compatible original')
    app = app_file.read_bytes()
    errors = fw.validate_app_binary(app)
    if errors:
        raise ValueError('FM1 application audit failed: ' + str(errors))
    regenerated = fw.build_flash_image_from_assets(
        app, assets['uboot'], assets['cfg_tool'], assets['eq_cfg'],
        assets['key'], isd_config=assets['isd_config'],
        flash_header_payload=assets['flash_header_payload'],
    )
    if len(old_flash) != len(regenerated):
        raise ValueError('Unexpected partition layout change')
    changed = [i for i in range(HEAD_BYTES) if old_flash[i] != regenerated[i]]
    if changed != [32, 33, 34, 35]:
        raise ValueError('Unexpected protected header mutation: ' + str(changed))
    flash = old_flash[:HEAD_BYTES] + regenerated[HEAD_BYTES:]
    package = fw.build_fwsc(flash, assets['ota'], 'FM-1_1015')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(package)
    report, errors = fw.verify_fwsc(output)
    if errors:
        output.unlink(missing_ok=True)
        raise ValueError('Output failed package verification: ' + str(errors))
    again = fw.parse_ufw(package)
    out_flash = next(e['data'] for e in again['entries'] if e['name'] == 'flash.bin')
    out_ota = next(e['data'] for e in again['entries'] if e['name'] == 'ota.bin')
    assert out_flash[:HEAD_BYTES] == old_flash[:HEAD_BYTES]
    assert out_ota == assets['ota']
    return {
        'version': '1.15', 'product': 'FM-1_1015',
        'status': 'EXPERIMENTAL LAB ONLY; physical FM1 and ROM restore unverified',
        'reference_sha256': PINNED_V114,
        'application_sha256': sha(app), 'package_sha256': sha(package),
        'package_bytes': len(package), 'boot_header_unchanged': True,
        'ota_loader_unchanged': True, 'offline_verifier_passed': report['valid'],
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('app', type=Path, help='FM1 pi32v2 app.bin')
    p.add_argument('v114', type=Path, help='SHA-pinned experimental V1.14 package')
    p.add_argument('out', type=Path, help='Result filename ending .fwsc.lab')
    args = p.parse_args()
    print(json.dumps(build(args.app, args.v114, args.out), indent=2))


if __name__ == '__main__':
    main()
