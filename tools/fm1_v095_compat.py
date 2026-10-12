#!/usr/bin/env python3
"""ETCH V0.95 *lab-only* compatibility packer for an FM1 currently on V0.89.

Crucial distinction: rebuild_flash_image_from_assets regenerates the top-level
JLFS directory. ETCH 0.89's physical first 16 KiB contain four directory bytes
that differ from the regenerated output. The installed 0.89 app compares the
complete first 16 KiB before staging OTA, so we must preserve the original
reference bytes rather than regenerate those four bytes. No boot-code edits.

This does NOT validate physical recovery or guarantee flashing safety.
"""
from __future__ import annotations
import argparse,hashlib,json
from pathlib import Path
import fm1_fwsc as F

KNOWN_V089_SHA = '330f35f839ef8e53a8a49bcf7bd5cf9f18838246357122c5390ee422c5023b97'
V095_PRODUCT='FM-1_995'
HEAD=0x4000

def sha(b): return hashlib.sha256(b).hexdigest()

def make_compat(app_path:Path,installed_path:Path,out_path:Path)->dict:
    if out_path.suffix != '.lab' or not out_path.name.endswith('.fwsc.lab'):
        raise ValueError('Only lab package *.fwsc.lab is permitted; physical R0 restore is unverified')
    before=installed_path.read_bytes()
    if sha(before)!=KNOWN_V089_SHA:
        raise ValueError('This packer supports only the EXACT V0.89 package supplied for this FM1. Refusing other boot headers.')
    parsed=F.parse_ufw(before)
    if parsed['product']!='FM-1_989':raise ValueError('Current reference is not the pinned V0.89 firmware')
    report,errors=F.verify_fwsc(installed_path)
    if errors:raise ValueError('Reference package verification failed: '+', '.join(errors))
    assets=F.extract_stock_assets(installed_path)
    original_flash=next(e['data'] for e in parsed['entries'] if e['name']=='flash.bin')
    app=app_path.read_bytes()
    errors=F.validate_app_binary(app)
    if errors: raise ValueError('App invalid: '+', '.join(errors))
    regenerated=F.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
    if len(regenerated)!=len(original_flash):raise ValueError('Reference flash layout changed')
    mismatch=[i for i,(a,b) in enumerate(zip(original_flash[:HEAD],regenerated[:HEAD])) if a!=b]
    # Freeze the exact, researched header difference. A wider change merits review.
    if mismatch!=[0x20,0x21,0x22,0x23]:
        raise ValueError(f'Unexpected protected-region mismatch at {[hex(i) for i in mismatch[:25]]}; aborting')
    combined=original_flash[:HEAD]+regenerated[HEAD:]
    assert combined[:HEAD]==original_flash[:HEAD]
    assert combined[HEAD:]==regenerated[HEAD:]
    package=F.build_fwsc(combined,assets['ota'],V095_PRODUCT)
    out_path.parent.mkdir(parents=True,exist_ok=True)
    out_path.write_bytes(package)
    report,errors=F.verify_fwsc(out_path)
    if errors:out_path.unlink(missing_ok=True);raise ValueError('Constructed package invalid: '+', '.join(errors))
    check=F.parse_ufw(out_path.read_bytes())
    new_flash=next(e['data'] for e in check['entries'] if e['name']=='flash.bin')
    assert new_flash[:HEAD] == original_flash[:HEAD]
    assert check['product']==V095_PRODUCT
    assert next(e['data'] for e in check['entries'] if e['name']=='ota.bin') == assets['ota']
    details={
        'status':'LAB ONLY / not physically validated',
        'firmware_product':V095_PRODUCT,
        'reference_product':parsed['product'],
        'reference_fwsc_sha256':sha(before),
        'protected_head_bytes':HEAD,
        'protected_head_sha256':sha(new_flash[:HEAD]),
        'identical_to_reference_head':True,
        'rebuilt_head_changed_offsets':[hex(i) for i in mismatch],
        'ota_loader_identical':True,
        'storage_flash_layout':'unchanged; project banks 0xB4000–0xDC000 outside packaged 0x93000 flash.bin',
        'app_sha256':sha(app),
        'package_bytes':len(package),
        'package_sha256':sha(package),
        'firmware_validation':report['valid'],
        'warning':'Matching a known 0.89 header addresses the CHECK mismatch only, not physical safety, flash restore or stage-two updater compatibility.'
    }
    out_path.with_suffix(out_path.suffix+'.compat.json').write_text(json.dumps(details,indent=2)+'\n')
    return details

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('app_bin',type=Path)
    p.add_argument('installed_v089_fwsc',type=Path)
    p.add_argument('out_fwsc_lab',type=Path)
    a=p.parse_args()
    print(json.dumps(make_compat(a.app_bin,a.installed_v089_fwsc,a.out_fwsc_lab),indent=2))
