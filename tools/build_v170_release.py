#!/usr/bin/env python3
"""Pack a single official ETCH V1.70 UAC release and hidden legacy head.
Preserves `FM-1_1500`, pinned 16KiB head, and exact V1.60 OTA loader.
USB audio is built with 44.1/48 kHz support; never claims physical validation.
"""
from __future__ import annotations
import base64, hashlib, json, re, shutil
from pathlib import Path
import fm1_fwsc as fw
ROOT=Path(__file__).resolve().parents[1]
REL=ROOT/'release'
APP=ROOT/'build/fm1-native-storage-updater-uac48-p05/app.bin'
PRODUCT='FM-1_1500'

def sha(b:bytes)->str:return hashlib.sha256(b).hexdigest()
def main():
    app=APP.read_bytes()
    if not app or len(app)>fw.APP_SLOT:raise ValueError('missing or oversized compiled USB audio application')
    results={}
    for variant,tag,refname,targetname in (
        ('etch','firmware-embedded','ETCH-FM1-V1.60-ETCH-HEAD.fwsc','ETCH-V1.70.fwsc'),
        ('stock','firmware-embedded-stockhead','ETCH-FM1-V1.60-STOCK-HEAD.fwsc','ETCH-V1.70-LEGACY-STOCK-HEAD.fwsc'),
    ):
        refpath=REL/refname
        ref,err=fw.verify_fwsc(refpath)
        if err or ref['product']!=PRODUCT:raise ValueError('bad pinned source '+str(err))
        assets=fw.extract_stock_assets(refpath)
        flash=fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],
            key=assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
        src=fw.parse_ufw(refpath.read_bytes())
        original_flash=next(e['data'] for e in src['entries'] if e['name']=='flash.bin')
        original_ota=next(e['data'] for e in src['entries'] if e['name']=='ota.bin')
        flash=original_flash[:0x4000]+flash[0x4000:]
        result=fw.build_fwsc(flash,original_ota,PRODUCT)
        target=(REL if variant=='etch' else ROOT/'compat')/targetname
        target.write_bytes(result)
        parsed,errors=fw.verify_fwsc(target)
        if errors or parsed['product']!=PRODUCT:raise ValueError(str(errors))
        entries=fw.parse_ufw(result)['entries']
        assert next(e['data'] for e in entries if e['name']=='ota.bin')==original_ota
        assert next(e['data'] for e in entries if e['name']=='flash.bin')[:0x4000]==original_flash[:0x4000]
        results[variant]={'file':str(target.relative_to(ROOT)),'sha256':sha(result),'bytes':len(result),
            'protected_boot_sha256':sha(original_flash[:0x4000]),'ota_sha256':sha(original_ota),
            'identity':PRODUCT,'type':'official' if variant=='etch' else 'webinstaller-legacy-only'}
        print('PASS',variant,target.name,len(result),sha(result),'product',parsed['product'])
    report={'release_version':'1.70','firmware_identity':PRODUCT,'app_sha256':sha(app),'app_bytes':len(app),
        'project_format':'PJT31','native_storage_enabled':True,'native_ota_enabled':True,
        'usb_audio_44100':True,'usb_audio_48000':True,'hardware_verified':False,
        'primary_fwsc':'release/ETCH-V1.70.fwsc','packages':results}
    (ROOT/'RELEASE-MANIFEST.json').write_text(json.dumps(report,indent=2)+'\n')
    (REL/'V1.70-RELEASE-MANIFEST.json').write_text(json.dumps(report,indent=2)+'\n')
    installer=ROOT/'WebInstaller.html'
    s=installer.read_text()
    for old,new in [('V1.61','V1.70'),('V161','V170')]:s=s.replace(old,new)
    for kind,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
        raw=(ROOT/results[kind]['file']).read_bytes()
        pat=r'(<script id="'+tag+r'" type="application/octet-stream">)\s*[^<]*?\s*(</script>)'
        s,n=re.subn(pat,lambda m:m.group(1)+'\n'+base64.b64encode(raw).decode()+'\n'+m.group(2),s)
        if n!=1:raise ValueError('missing installer tag '+tag)
    for field,kind in [('HASH_ET','etch'),('HASH_ET_STOCKHEAD','stock')]:
        s,n=re.subn(field+r": '[0-9a-f]{64}'",field+": '"+results[kind]['sha256']+"'",s)
        if n!=1:raise ValueError('missing SHA256 field '+field)
    if "ID_ET: 'FM-1_1500'" not in s:raise ValueError('installer identity drift')
    installer.write_text(s)
    for kind,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
        payload=base64.b64decode(re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*([^<]+?)\s*</script>',s).group(1))
        if sha(payload)!=results[kind]['sha256']:raise ValueError('embedded mismatch')
    (REL/'ETCH-WebInstaller-V1.70.html').write_text(s)
    print('PASS packaged + WebInstaller verified: primary single official fwsc, legacy hidden, immutable identity')
if __name__=='__main__':main()
