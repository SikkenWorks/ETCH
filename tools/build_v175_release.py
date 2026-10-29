#!/usr/bin/env python3
"""Pack ETCH V1.75 using original V1.60 immutable identity, headers and loader.
Offline packaging checks only, not hardware flash evidence.
"""
from __future__ import annotations
import base64, hashlib, json, re
from pathlib import Path
import fm1_fwsc as fw
ROOT=Path(__file__).resolve().parents[1]
REL=ROOT/'release'
APP=ROOT/'build/fm1-native-storage-updater-uac48-p05/app.bin'
PRODUCT='FM-1_1500'
def sha(b:bytes)->str:return hashlib.sha256(b).hexdigest()
def main():
 app=APP.read_bytes()
 if not app or len(app)>fw.APP_SLOT:raise ValueError('missing or oversized USB app')
 result={}
 for variant,refname,target in (
  ('etch','ETCH-FM1-V1.60-ETCH-HEAD.fwsc',REL/'ETCH-V1.75.fwsc'),
  ('stock','ETCH-FM1-V1.60-STOCK-HEAD.fwsc',ROOT/'compat/ETCH-V1.75-LEGACY-STOCK-HEAD.fwsc'),
 ):
  refpath=REL/refname
  ref,err=fw.verify_fwsc(refpath)
  if err or ref['product']!=PRODUCT:raise ValueError('bad pinned source '+str(err))
  assets=fw.extract_stock_assets(refpath)
  flash=fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],
   key=assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
  src=fw.parse_ufw(refpath.read_bytes())
  old_flash=next(e['data'] for e in src['entries'] if e['name']=='flash.bin')
  ota=next(e['data'] for e in src['entries'] if e['name']=='ota.bin')
  flash=old_flash[:0x4000]+flash[0x4000:]
  payload=fw.build_fwsc(flash,ota,PRODUCT)
  target.write_bytes(payload)
  checked,errors=fw.verify_fwsc(target)
  if errors or checked['product']!=PRODUCT:raise ValueError('firmware verification failed '+str(errors))
  entries=fw.parse_ufw(payload)['entries']
  assert next(e['data'] for e in entries if e['name']=='ota.bin')==ota
  assert next(e['data'] for e in entries if e['name']=='flash.bin')[:0x4000]==old_flash[:0x4000]
  result[variant]={'file':str(target.relative_to(ROOT)),'sha256':sha(payload),'bytes':len(payload),
   'protected_boot_sha256':sha(old_flash[:0x4000]),'ota_sha256':sha(ota),'identity':PRODUCT,
   'type':'official' if variant=='etch' else 'installer-legacy-only'}
  print('PASS package',variant,target.name,len(payload),sha(payload),checked['product'])
 manifest={'release_version':'1.75','firmware_identity':PRODUCT,'app_sha256':sha(app),'app_bytes':len(app),
  'project_format':'PJT31','native_storage_enabled':True,'native_ota_enabled':True,
  'usb_audio_44100':True,'usb_audio_48000':True,'hardware_verified':False,
  'primary_fwsc':'release/ETCH-V1.75.fwsc','packages':result}
 (ROOT/'RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
 (REL/'V1.75-RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
 installer=ROOT/'WebInstaller.html'
 s=installer.read_text()
 s=s.replace('V1.74','V1.75').replace('V174','V175')
 for kind,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
  raw=(ROOT/result[kind]['file']).read_bytes()
  pat=r'(<script id="'+tag+r'" type="application/octet-stream">)\s*[^<]*?\s*(</script>)'
  s,n=re.subn(pat,lambda m:m.group(1)+'\n'+base64.b64encode(raw).decode()+'\n'+m.group(2),s)
  if n!=1:raise ValueError('missing embedded firmware '+tag)
 for field,kind in [('HASH_ET','etch'),('HASH_ET_STOCKHEAD','stock')]:
  s,n=re.subn(field+r": '[0-9a-f]{64}'",field+": '"+result[kind]['sha256']+"'",s)
  if n!=1:raise ValueError('missing SHA '+field)
 if "ID_ET: 'FM-1_1500'" not in s:raise ValueError('product drift')
 s=s.replace('LCD EFFICIENCY + STABILITY','SHARED UNDO MEMORY + STABILITY')
 s=s.replace('V1.75: bounded LCD rendering with preserved 62-screen catalog and STRING ENS, original starters preserved, and USB-audio-priority LCD updates.',
  'V1.75: 12,288 bytes of static DSP RAM reclaimed using a shared 16-step undo/redo history; 62 screens, STRING ENS, FX and USB audio retained.')
 s=s.replace('V1.75 uses the B1 HF2 OTA transition for Felucca/SLOOP FM-1_900, including ota-FM-1_9xx loader recognition. The correct protected header is selected separately, including the verified V15 header for ETCH V1.34–V1.38. V1.48 includes two strictly verified ETCH package variants for the existing ETCH and original V15 boot headers. The audio DSP stays unchanged from V1.47. Physical cross-firmware flashing remains unverified.',
  'V1.75 reduces DSP RAM usage with no removed features. The app-only OTA path retains the fixed ETCH identity and its trusted loader, while the WebInstaller can select a legacy protected header for supported older installations. Hardware OTA safety and cross-firmware transitions remain unverified.')
 
 installer.write_text(s)
 for kind,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
  match=re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',s)
  if not match or sha(base64.b64decode(match.group(1)))!=result[kind]['sha256']:
   raise ValueError('embedded firmware SHA drift')
 (REL/'ETCH-WebInstaller-V1.75.html').write_text(s)
 print('PASS single official ETCH-V1.75.fwsc + hidden legacy head, firmware/installer SHA-256 parity')
if __name__=='__main__':main()
