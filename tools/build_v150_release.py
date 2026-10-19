#!/usr/bin/env python3
"""Build ETCH V1.50 from the *pinned* V1.48.5 B1 HF2 OTA assets.
Only the app partition is replaced; original protected bootheads and loader
remain untouched. Requires a native pi32v2 build using --storage --updater.
"""
from pathlib import Path
import sys,hashlib,base64,re,json
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
R=Path(__file__).resolve().parents[1]
H=lambda b:hashlib.sha256(b).hexdigest()
ref=R/'compat/V1.48.5-OTA-VERIFIED-BASE.fwsc'
v15=R/'compat/V15-VERIFIED-BOOTHEAD.bin'
app=(R/'build/fm1-native-storage-updater-p05/app.bin').read_bytes()
if b'FM-1_1500' not in app or fw.validate_app_binary(app):
 sys.exit('BLOCK: missing or invalid V1.50 native application; build firmware first')
refinfo,errors=fw.verify_fwsc(ref)
if errors or refinfo['product']!='FM-1_1485':sys.exit('BLOCK: reference image is not the checked V1.48.5 firmware')
assets=fw.extract_stock_assets(ref)
raw=next(e['data'] for e in fw.parse_ufw(ref.read_bytes())['entries'] if e['name']=='flash.bin')
etchhead=raw[:0x4000];stockhead=v15.read_bytes()
if H(etchhead)!='a65e0e1a2fad1db943c00b529b45553c524257146145db61b69628cefcf2b2c9':sys.exit('BLOCK: unexpected ETCH header')
if H(stockhead)!='d9f43191777656de01c7bed4f9e9ba2e34e94832c5d2ed02c3fc7ab7a6d9bd67' or len(stockhead)!=0x4000:sys.exit('BLOCK: unexpected V15 header')
image=fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
if len(image)!=len(raw):sys.exit('BLOCK: flash layout drift')
variants={}
for label,head in (('etch',etchhead),('stock',stockhead)):
 blob=fw.build_fwsc(head+image[0x4000:],assets['ota'],'FM-1_1500')
 dest=R/'release'/f'ETCH-FM1-V1.50-{label.upper()}-HEAD.fwsc';dest.parent.mkdir(exist_ok=True)
 dest.write_bytes(blob)
 info,errors=fw.verify_fwsc(dest)
 if errors or info['product']!='FM-1_1500':sys.exit(f'BLOCK: {label} firmware verify {errors}')
 parsed=next(e['data'] for e in fw.parse_ufw(blob)['entries'] if e['name']=='flash.bin')
 if parsed[:0x4000]!=head or parsed[0x4000:]!=image[0x4000:]:sys.exit('BLOCK: flash body mismatch')
 if fw.extract_stock_assets(dest)['ota']!=assets['ota']:sys.exit('BLOCK: OTA loader drift')
 variants[label]={'file':str(dest.relative_to(R)),'sha256':H(blob),'bootheader_sha256':H(head),'bytes':len(blob)}
# Use the canonical HF2 installer as template, idempotently replace payload and pins.
original=R/'WebInstaller.html'
html=original.read_text()
for label,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 blob=(R/variants[label]['file']).read_bytes()
 patt=rf'(<script id="{tag}" type="application/octet-stream">)\s*.*?(</script>)'
 html,n=re.subn(patt,lambda m:m.group(1)+'\n'+base64.b64encode(blob).decode()+'\n'+m.group(2),html,count=1,flags=re.S)
 if n!=1:sys.exit('BLOCK: missing firmware embedded marker '+tag)
for tag,key in [('HASH_ET','etch'),('HASH_ET_STOCKHEAD','stock')]:
 pat=r'('+tag+r": ')[0-9a-f]{64}'"
 html,n=re.subn(pat,lambda m:m.group(1)+variants[key]['sha256']+"'",html,count=1)
 if n!=1:sys.exit('BLOCK: missing firmware pin '+tag)
html=html.replace('FM-1_1490','FM-1_1500').replace('V1.49','V1.50').replace('1.49','1.50')
for label,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 b64=re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*(.*?)\s*</script>',html,re.S)
 if not b64 or base64.b64decode(b64.group(1))!=(R/variants[label]['file']).read_bytes():sys.exit('BLOCK: installer parity failed')
(R/'WebInstaller.html').write_text(html)
manifest={'version':'1.50','firmware_identity':'FM-1_1500','native_ota_enabled':True,'native_storage_enabled':True,'project_format':'PJT31','firmware_variants':variants,'application_sha256':H(app),'application_bytes':len(app),'ota_loader_sha256':H(assets['ota']),'audio_pcm_bit_exact_vs_v1485':True,'outgoing_app_only_ota':'native host regression passed; original Felucca releases and hardware pending','stock_v15_restore':'exact pinned stock image only, physical transition not verified','hardware_verified':False,'installer_basis':'B1 V4.3 HF2 ported in ETCH V1.48.5'}
(R/'RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
(R/'release/V1.50-RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
