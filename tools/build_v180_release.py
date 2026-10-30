#!/usr/bin/env python3
"""Repackage an already cross-built ETCH V1.80 app without altering FM1 identity,
protected 16 KiB bootheader, OTA executable or the supported UFW container.
This never communicates with a physical device. Hardware tests still required.
"""
import sys,json,hashlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
root=Path(__file__).resolve().parents[1]
app=(root/'build/fm1-native-storage-updater-uac48-p05/app.bin').read_bytes()
assert not fw.validate_app_binary(app), 'invalid native app'
manifest={'release_version':'1.80','firmware_identity':'FM-1_1500','project_format':'PJT31',
 'app_sha256':hashlib.sha256(app).hexdigest(),'app_bytes':len(app),'native_storage_enabled':True,
 'native_ota_enabled':True,'usb_audio_44100':True,'usb_audio_48000':True,
 'hardware_verified':False,'primary_fwsc':'release/ETCH-V1.80.fwsc','packages':{}}
for kind,name,ref in [('etch','release/ETCH-V1.80.fwsc','release/ETCH-V1.79.fwsc'),
                       ('stock','compat/ETCH-V1.80-LEGACY-STOCK-HEAD.fwsc','compat/ETCH-V1.79-LEGACY-STOCK-HEAD.fwsc')]:
 old=root/ref
 prev,err=fw.verify_fwsc(old)
 assert not err and prev['product']=='FM-1_1500',(err,prev)
 assets=fw.extract_stock_assets(old)
 old_entries=fw.parse_ufw(old.read_bytes())['entries']
 old_flash=next(e['data'] for e in old_entries if e['name']=='flash.bin')
 old_loader=next(e['data'] for e in old_entries if e['name']=='ota.bin')
 flash=fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],
 key=assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
 flash=old_flash[:0x4000]+flash[0x4000:]
 package=fw.build_fwsc(flash,old_loader,'FM-1_1500')
 out=root/name;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(package)
 test,errors=fw.verify_fwsc(out)
 assert not errors and test['product']=='FM-1_1500',(errors,test)
 newentries=fw.parse_ufw(package)['entries']
 newflash=next(e['data'] for e in newentries if e['name']=='flash.bin')
 newloader=next(e['data'] for e in newentries if e['name']=='ota.bin')
 assert newflash[:0x4000]==old_flash[:0x4000]
 assert newloader==old_loader
 H=lambda x:hashlib.sha256(x).hexdigest()
 manifest['packages'][kind]={'file':name,'sha256':H(package),'bytes':len(package),
 'protected_boot_sha256':H(newflash[:0x4000]),'ota_sha256':H(newloader),
 'identity':test['product'],'type':'official' if kind=='etch' else 'installer-legacy-only'}
 print('PASS',kind,name,len(package),'SHA256',H(package))
(root/'RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
(root/'release/V1.80-RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
