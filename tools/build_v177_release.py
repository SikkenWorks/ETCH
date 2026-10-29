#!/usr/bin/env python3
"""Pack ETCH V1.77 using the original V1.60 immutable identity, headers and loader.
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
  ('etch','ETCH-FM1-V1.60-ETCH-HEAD.fwsc',REL/'ETCH-V1.77.fwsc'),
  ('stock','ETCH-FM1-V1.60-STOCK-HEAD.fwsc',ROOT/'compat/ETCH-V1.77-LEGACY-STOCK-HEAD.fwsc'),
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
 manifest={'release_version':'1.77','firmware_identity':PRODUCT,'app_sha256':sha(app),'app_bytes':len(app),
  'project_format':'PJT31','native_storage_enabled':True,'native_ota_enabled':True,
  'usb_audio_44100':True,'usb_audio_48000':True,'hardware_verified':False,
  'primary_fwsc':'release/ETCH-V1.77.fwsc','packages':result}
 (ROOT/'RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
 (REL/'V1.77-RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
 installer=ROOT/'WebInstaller.html'
 s=installer.read_text()
 s=s.replace('V1.76','V1.77').replace('V176','V177')
 for kind,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
  raw=(ROOT/result[kind]['file']).read_bytes()
  pat=r'(<script id="'+tag+r'" type="application/octet-stream">)\s*[^<]*?\s*(</script>)'
  s,n=re.subn(pat,lambda m:m.group(1)+'\n'+base64.b64encode(raw).decode()+'\n'+m.group(2),s)
  if n!=1:raise ValueError('missing embedded firmware '+tag)
 for field,kind in [('HASH_ET','etch'),('HASH_ET_STOCKHEAD','stock')]:
  s,n=re.subn(field+r": '[0-9a-f]{64}'",field+": '"+result[kind]['sha256']+"'",s)
  if n!=1:raise ValueError('missing SHA '+field)
 if "ID_ET: 'FM-1_1500'" not in s:raise ValueError('product drift')
 s=s.replace('SMOOTH ORGANIC DRIFT + STABILITY','PRESET BACKUP + SAVE FEEDBACK')
 s=s.replace('V1.77: ORGANIC pitch drift now bends continuously over 2.5–9 seconds with smooth start/end; 62 screens, STRING ENS, RAM recovery and USB audio preserved.',
  'V1.77: RAM-only sound backup via USB-MIDI, .etchpreset import/export and SAVE / VERIFYING status. Existing sounds, screens and projects retained.')
 s=s.replace('V1.77 removes abrupt ORGANIC drift jumps with zero additional static RAM and no removed features.',
  'V1.77 introduces a RAM-only preset snapshot protocol and save feedback. Previous ORGANIC smoothing and all native features remain present.')
 installer.write_text(s)
 for kind,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
  match=re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',s)
  if not match or sha(base64.b64decode(match.group(1)))!=result[kind]['sha256']:
   raise ValueError('embedded firmware SHA drift')
 (REL/'ETCH-WebInstaller-V1.77.html').write_text(s)
 print('PASS single official ETCH-V1.77.fwsc + hidden legacy head, firmware/installer SHA-256 parity')
if __name__=='__main__':main()
