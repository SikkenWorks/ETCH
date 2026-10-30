#!/usr/bin/env python3
"""Pack ETCH V1.86 with original FM-1_1500 identity, unchanged protected
bootheader and unchanged trusted loader from the corresponding V1.83 container.
Software-only packaging; no physical device interaction.
"""
import sys,json,hashlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
root=Path(__file__).resolve().parents[1]
app=(root/'build/fm1-native-storage-updater-uac48-p05/app.bin').read_bytes()
assert not fw.validate_app_binary(app),'invalid target application'
H=lambda b:hashlib.sha256(b).hexdigest()
manifest={'release_version':'1.86','firmware_identity':'FM-1_1500','project_format':'PJT32',
          'app_sha256':H(app),'app_bytes':len(app),'native_storage_enabled':True,
          'native_ota_enabled':True,'usb_audio_44100':True,'usb_audio_48000':True,
          'hardware_verified':False,'primary_fwsc':'release/ETCH-V1.86.fwsc','packages':{}}
variants=[('etch','release/ETCH-V1.86.fwsc','release/ETCH-V1.85.fwsc'),
          ('stock','compat/ETCH-V1.86-LEGACY-STOCK-HEAD.fwsc','compat/ETCH-V1.85-LEGACY-STOCK-HEAD.fwsc')]
for kind,name,reference in variants:
  old=root/reference
  previous,errors=fw.verify_fwsc(old)
  assert not errors and previous['product']=='FM-1_1500',(reference,errors,previous)
  assets=fw.extract_stock_assets(old)
  entries=fw.parse_ufw(old.read_bytes())['entries']
  old_flash=next(e['data'] for e in entries if e['name']=='flash.bin')
  old_loader=next(e['data'] for e in entries if e['name']=='ota.bin')
  flash=fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],
              key=assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
  flash=old_flash[:0x4000]+flash[0x4000:]
  package=fw.build_fwsc(flash,old_loader,'FM-1_1500')
  path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(package)
  current,errors=fw.verify_fwsc(path)
  assert not errors and current['product']=='FM-1_1500',(name,errors,current)
  parts=fw.parse_ufw(package)['entries']
  new_flash=next(e['data'] for e in parts if e['name']=='flash.bin')
  new_loader=next(e['data'] for e in parts if e['name']=='ota.bin')
  assert new_flash[:0x4000]==old_flash[:0x4000]
  assert new_loader==old_loader
  manifest['packages'][kind]={'file':name,'sha256':H(package),'bytes':len(package),
      'protected_boot_sha256':H(new_flash[:0x4000]),'ota_sha256':H(new_loader),
      'identity':current['product'],'type':'official' if kind=='etch' else 'installer-legacy-only'}
  print('PASS',kind,name,len(package),'SHA-256:',H(package))
(root/'RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
(root/'release/V1.86-RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
