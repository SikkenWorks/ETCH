#!/usr/bin/env python3
"""Reproducibly package ETCH 1.60 using exact ETCH 1.50 product identity.

No hardware operations; does not claim OTA installation has been verified.
The 16 KiB protected flash region and OTA payload are byte-identical to the
chosen V1.50 reference package; only encrypted application bytes are changed.
"""
from __future__ import annotations
import hashlib, json, sys
from pathlib import Path
import fm1_fwsc as fw
ROOT = Path(__file__).resolve().parents[1]
RELEASE = ROOT/'release'
APP = ROOT/'build/fm1-native-storage-updater-p05/app.bin'
REFS = {
    'etch': RELEASE/'ETCH-FM1-V1.50-ETCH-HEAD.fwsc',
    'stock': RELEASE/'ETCH-FM1-V1.50-STOCK-HEAD.fwsc',
}
PRODUCT = 'FM-1_1500'  # DO NOT increment this with the display/release version.

def sha(data): return hashlib.sha256(data).hexdigest()

def main():
    app=APP.read_bytes()
    if not app or len(app)>fw.APP_SLOT:raise ValueError('compiled FM1 app missing or exceeds slot')
    report={'release_version':'1.60','firmware_identity':PRODUCT,'native_ota_enabled':True,
            'native_storage_enabled':True,'project_format':'PJT31',
            'app_sha256':sha(app),'app_bytes':len(app),'hardware_verified':False,
            'source_variant':'firmware/fm1-native/build.py --storage --updater',
            'cross_firmware_tests':'native synthetic container replay; external Felucca packages and FM1 hardware not tested',
            'packages':{}}
    for variant,ref_path in REFS.items():
        raw=ref_path.read_bytes()
        ref,errors=fw.verify_fwsc(ref_path)
        if errors or ref['product']!=PRODUCT:raise ValueError(f'{ref_path}: {errors} product {ref["product"]}')
        assets=fw.extract_stock_assets(ref_path)
        flash=fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],
                 key=assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
        source=fw.parse_ufw(raw)
        original_flash=next(e['data'] for e in source['entries'] if e['name']=='flash.bin')
        original_ota=next(e['data'] for e in source['entries'] if e['name']=='ota.bin')
        # The stock-header variant intentionally carries the exact protected
        # bytes from V1.50, not a regenerated JieLi default boot directory.
        # Application bytes are already encrypted for the verified same key.
        flash = original_flash[:0x4000] + flash[0x4000:]
        assert flash[:0x4000]==original_flash[:0x4000]
        result=fw.build_fwsc(flash,original_ota,PRODUCT)
        path=RELEASE/f'ETCH-FM1-V1.60-{variant.upper()}-HEAD.fwsc'
        path.write_bytes(result)
        verified,errors=fw.verify_fwsc(path)
        if errors or verified['product']!=PRODUCT:raise ValueError(f'packaging validation failed {path.name}: {errors}')
        if next(e['data'] for e in fw.parse_ufw(result)['entries'] if e['name']=='ota.bin')!=original_ota:
            raise AssertionError('OTA executable changed unexpectedly')
        report['packages'][variant]={
            'file':str(path.relative_to(ROOT)),'sha256':sha(result),'bytes':len(result),
            'protected_boot_sha256':sha(flash[:0x4000]), 'original_package_identity':verified['product'],
            'ota_sha256':sha(original_ota), 'ota_executable_preserved':True,
        }
        print(f'PASS {variant}: {path.name} ({len(result)} bytes) SHA256={sha(result)} product={PRODUCT}')
    manifest=ROOT/'RELEASE-MANIFEST.json'
    manifest.write_text(json.dumps(report,indent=2)+'\n')
    (RELEASE/'V1.60-RELEASE-MANIFEST.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS compiled app, unchanged 16KiB protected heads, unchanged OTA loader, consistent identity')

if __name__=='__main__':main()
