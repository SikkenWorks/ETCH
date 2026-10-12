#!/usr/bin/env python3
"""Prepare V1.20 LAB firmware using V1.19's EXACT protected 16 KiB header.

Only makes a .fwsc.lab test artifact, does NOT communicate with a device.
Refuses to create ordinary flashable .fwsc without independent recovery proof.
Usage: python pack_v119_preserve_head.py APP.bin V119.fwsc.lab OUT.fwsc.lab
"""
from pathlib import Path
import argparse,hashlib
import fm1_fwsc as fw

def pack(app:Path,reference:Path,out:Path):
 if not out.name.endswith('.fwsc.lab'):
  raise ValueError('Only .fwsc.lab development output is permitted')
 ra,re=fw.verify_fwsc(reference)
 if re or ra['product']!='FM-1_1019':
  raise ValueError('Required verified FM-1_1019 V1.19 reference; '+str(re))
 if fw.validate_app_binary(app.read_bytes()):
  raise ValueError('Bad application binary')
 assets=fw.extract_stock_assets(reference)
 ref_flash=next(e['data'] for e in fw.parse_ufw(reference.read_bytes())['entries'] if e['name']=='flash.bin')
 flash=bytearray(fw.build_flash_image_from_assets(
  app.read_bytes(),assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],
  assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload']))
 flash[:0x4000]=ref_flash[:0x4000]
 out.write_bytes(fw.build_fwsc(bytes(flash),assets['ota'],'FM-1_1020'))
 report,errors=fw.verify_fwsc(out)
 if errors or report['product']!='FM-1_1020':
  out.unlink(missing_ok=True)
  raise ValueError('Package verification failed '+str(errors))
 again=fw.parse_ufw(out.read_bytes())
 rebuilt=next(e['data'] for e in again['entries'] if e['name']=='flash.bin')
 out_ota=next(e['data'] for e in again['entries'] if e['name']=='ota.bin')
 if rebuilt[:0x4000]!=ref_flash[:0x4000] or out_ota!=assets['ota']:
  out.unlink(missing_ok=True)
  raise ValueError('Critical OTA/protected 16KiB mismatch')
 print('LAB package verified; V1.19 protected header and OTA EXACT MATCH')
 print('SHA-256',hashlib.sha256(out.read_bytes()).hexdigest())

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('app',type=Path);p.add_argument('reference',type=Path);p.add_argument('out',type=Path)
 a=p.parse_args();pack(a.app,a.reference,a.out)
