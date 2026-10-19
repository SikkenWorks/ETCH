#!/usr/bin/env python3
"""Make new FM-1_1485 binaries using verified previous V1.48 OTA assets.

This is a software-only package producer; hardware flash safety is not established.
"""
from pathlib import Path
import base64, hashlib, json, re, sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
R=Path(__file__).resolve().parents[1]
sha=lambda b:hashlib.sha256(b).hexdigest()
old={k:R/'release'/f'ETCH-FM1-V1.48-{k.upper()}-HEAD.fwsc' for k in ('etch','stock')}
app=(R/'build/fm1-native-storage-updater-p05/app.bin').read_bytes()
if b'FM-1_1485' not in app or b'FM-1_1048' in app or fw.validate_app_binary(app):
 raise SystemExit('Wrong application binary, rebuild new V1.48.5 app first')
refs={}
for name,path in old.items():
 inf,errors=fw.verify_fwsc(path)
 if errors or inf['product']!='FM-1_1048':raise SystemExit(f'Invalid historical reference: {name} {errors}')
 refs[name]=path.read_bytes()
refassets=fw.extract_stock_assets(old['etch'])
rawflash=lambda blob:next(e['data'] for e in fw.parse_ufw(blob)['entries'] if e['name']=='flash.bin')
oldflash={name:rawflash(blob) for name,blob in refs.items()}
if sha(oldflash['etch'][:0x4000])!='a65e0e1a2fad1db943c00b529b45553c524257146145db61b69628cefcf2b2c9':raise SystemExit('Unexpected ETCH protected header')
if sha(oldflash['stock'][:0x4000])!='d9f43191777656de01c7bed4f9e9ba2e34e94832c5d2ed02c3fc7ab7a6d9bd67':raise SystemExit('Unexpected V15 protected header')
rebuilt=fw.build_flash_image_from_assets(app,refassets['uboot'],refassets['cfg_tool'],refassets['eq_cfg'],refassets['key'],isd_config=refassets['isd_config'],flash_header_payload=refassets['flash_header_payload'])
if len(rebuilt)!=len(oldflash['etch']) or len(rebuilt)!=len(oldflash['stock']):raise SystemExit('Flash layout drift')
if rebuilt[0x4000:0x4000+16]!=oldflash['etch'][0x4000:0x4000+16]:
 print('INFO: app area contains new bytes, as intended')
variants={}
for label in ('etch','stock'):
 data=fw.build_fwsc(oldflash[label][:0x4000]+rebuilt[0x4000:],refassets['ota'],'FM-1_1485')
 target=R/'release'/f'ETCH-FM1-V1.48.5-{label.upper()}-HEAD.fwsc'
 target.write_bytes(data)
 info,errors=fw.verify_fwsc(target)
 if errors or info['product']!='FM-1_1485':raise SystemExit(f'New {label} fwsc validation failed: {errors}')
 actual=rawflash(data)
 if actual[:0x4000]!=oldflash[label][:0x4000] or actual[0x4000:]!=rebuilt[0x4000:]:raise SystemExit('Rebuilt flash changed outside allowed regions')
 if fw.extract_stock_assets(target)['ota']!=refassets['ota']:raise SystemExit('OTA-loader drift')
 variants[label]={'file':str(target.relative_to(R)),'sha256':sha(data),'head_sha256':sha(actual[:0x4000]),'size':len(data)}
# Update the B1 HF2 installer binary payloads AND pinned hashes and expected version identity.
page=R/'WebInstaller.html'
html=page.read_text()
for label,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 blob=(R/variants[label]['file']).read_bytes()
 patt=rf'(<script id="{tag}" type="application/octet-stream">)\s*.*?(</script>)'
 html,n=re.subn(patt,lambda m:m.group(1)+'\n'+base64.b64encode(blob).decode()+'\n'+m.group(2),html,count=1,flags=re.S)
 if n!=1:raise SystemExit('Cannot find embedded '+tag)
for tag,newval in [('HASH_ET',variants['etch']['sha256']),('HASH_ET_STOCKHEAD',variants['stock']['sha256'])]:
 html,n=re.subn(r'('+tag+r": ')[0-9a-f]{64}'",r'\g<1>'+newval+"'",html,count=1)
 if n!=1:raise SystemExit('Cannot update pin '+tag)
if html.count("ID_ET: 'FM-1_1048'")!=1:raise SystemExit('Old expected identity missing')
html=html.replace("ID_ET: 'FM-1_1048'","ID_ET: 'FM-1_1485'")
html=html.replace('V1.48.4 B1 HF2 TRANSITION','V1.48.5 NEW FIRMWARE · B1 HF2')
html=html.replace('V1.48.4 SAFE HANDOFF','V1.48.5 NEW FIRMWARE')
html=html.replace('V1.48.4: uses','V1.48.5: uses')
html=html.replace('ETCH Web Installer V1.48.4','ETCH Web Installer V1.48.5')
html=html.replace('The ETCH firmware is unchanged.','New native firmware build FM-1_1485; synthesis behavior unchanged.')
html=html.replace('V1.48 FIRMWARE · INSTALLER UPDATE','NEW FM-1_1485 FIRMWARE')
html=html.replace('V1.48.4 bootheader compatibility diagnostics','V1.48.5 bootheader compatibility diagnostics')
html=html.replace('V1.48.4 B1 HF2 OTA','V1.48.5 B1 HF2 OTA')
for path in (page,R/'ETCH-WebInstaller-V1.48.5.html'):
 path.write_text(html)
for label,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 extracted=base64.b64decode(re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*(.*?)\s*</script>',html,re.S).group(1))
 if extracted!=(R/variants[label]['file']).read_bytes():raise SystemExit('Embedded payload mismatch')
manifest={'version':'1.48.5','firmware_identity':'FM-1_1485','firmware_variants':variants,'application_sha256':sha(app),'application_bytes':len(app),'ota_loader_unchanged':True,'native_storage_enabled':True,'native_ota_enabled':True,'project_format':'PJT31','synthesis_behavior_changed':False,'physically_tested':False,'source':'V1.48.4 B1 HF2, new app compilation and new fwsc payload'}
(R/'release/V1.48.5-RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
(R/'RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
