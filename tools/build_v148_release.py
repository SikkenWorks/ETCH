#!/usr/bin/env python3
"""Reproducible ETCH 1.48 app-only OTA packaging.

Both ETCH-head and original V15-head variants have EXACTLY the same
compiled FM1 application bytes and the pinned community app-only OTA loader.
Requires an independently obtained, exact official 2026-07-30 FM-1 V15 .fwsc.
No hardware is accessed, and no source bootloader is overwritten.
"""
from pathlib import Path
import argparse,base64,hashlib,json,re,sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
R=Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--stock-v15',type=Path,required=True,help='Unmodified official FM-1 V15 firmware (SHA-256 pinned)')
args=ap.parse_args()
sha=lambda b:hashlib.sha256(b).hexdigest()
stock=args.stock_v15.read_bytes()
if sha(stock)!='db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a':
 raise SystemExit('Refusing unknown stock package. Only original V15 is a verified recovery reference.')
reference=R/'compat/ETCH-V1.40-OTA-BASE-REFERENCE.fwsc'
info,errors=fw.verify_fwsc(reference)
if errors:raise SystemExit('ETCH OTA reference invalid: '+str(errors))
assets=fw.extract_stock_assets(reference)
fl=lambda raw:next(e['data'] for e in fw.parse_ufw(raw)['entries'] if e['name']=='flash.bin')
a=R/'build/fm1-native-storage-updater-p05/app.bin'
app=a.read_bytes()
if not b'FM-1_1048' in app or fw.validate_app_binary(app):raise SystemExit('No verified FM-1_1048 application; build --storage --updater first')
legacy=fl(reference.read_bytes())
orig=fl(stock)
if assets['key']!=fw.parse_flash(orig)['chip_key'] or fw.parse_flash(legacy)['chip_key']!=assets['key']:
 raise SystemExit('Chip keys do not match, no safe cross-head package possible')
def layout(flash):
 return [(e['name'],e['offset'],e['size']) for e in fw.parse_flash(flash)['top_entries']]
if layout(orig)!=layout(legacy):raise SystemExit('Stock and ETCH top-level flash layouts differ')
if sha(legacy[:0x4000])!='a65e0e1a2fad1db943c00b529b45553c524257146145db61b69628cefcf2b2c9' or sha(orig[:0x4000])!='d9f43191777656de01c7bed4f9e9ba2e34e94832c5d2ed02c3fc7ab7a6d9bd67':
 raise SystemExit('Unrecognized protected head. Aborting.')
flash=fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
if len(flash)!=len(orig) or len(flash)!=len(legacy):raise SystemExit('Flash size drift')
variants={}
for label,old in [('etch',legacy),('stock',orig)]:
 output=R/'release'/f'ETCH-FM1-V1.48-{label.upper()}-HEAD.fwsc'
 output.parent.mkdir(exist_ok=True)
 blob=fw.build_fwsc(old[:0x4000]+flash[0x4000:],assets['ota'],'FM-1_1048')
 output.write_bytes(blob)
 pkg,errors=fw.verify_fwsc(output)
 if errors or pkg['product']!='FM-1_1048':raise SystemExit('Invalid '+label+' package '+str(errors))
 flash_rebuilt=fl(blob)
 if flash_rebuilt[0x4000:]!=flash[0x4000:] or flash_rebuilt[:0x4000]!=old[:0x4000]:
  raise SystemExit('Header/app bytes do not match '+label)
 variants[label]={'file':str(output.relative_to(R)),'sha256':sha(blob),'head_sha256':sha(old[:0x4000]),'size':len(blob)}
(R/'release/V1.48-VARIANTS.json').write_text(json.dumps(variants,indent=2)+'\n')
# Replace BOTH embedded base64 payloads, and their SHA-256/length gates.
page=R/'WebInstaller.html';html=page.read_text()
for label,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 blob=(R/variants[label]['file']).read_bytes()
 regex=rf'(<script id="{tag}" type="application/octet-stream">)\s*.*?(</script>)'
 html,n=re.subn(regex,lambda m:m.group(1)+'\n'+base64.b64encode(blob).decode()+'\n'+m.group(2),html,count=1,flags=re.S)
 if n!=1:raise SystemExit('No embedded '+tag+' payload slot')
for name,value in [('HASH_ET',variants['etch']['sha256']),('HASH_ET_STOCKHEAD',variants['stock']['sha256']),('KNOWN_HEAD_HASH',variants['etch']['head_sha256']),('STOCK_HEAD_HASH',variants['stock']['head_sha256'])]:
 html,n=re.subn(r'('+name+r": ')[0-9a-f]{64}'",r'\g<1>'+value+"'",html,count=1)
 if n!=1:raise SystemExit('Failed to update '+name)
for path in [page,R/'ETCH-WebInstaller-V1.48.html']:
 path.write_text(html)
import html as htmlmodule
for label,tag in [('etch','firmware-embedded'),('stock','firmware-embedded-stockhead')]:
 b64=re.search(r'<script id="'+tag+r'" type="application/octet-stream">\s*(.*?)\s*</script>',html,re.S).group(1)
 if base64.b64decode(b64)!=(R/variants[label]['file']).read_bytes():raise SystemExit('Embedded package drift')
manifest={'version':'1.48','firmware_identity':'FM-1_1048','firmware_file':variants['etch']['file'],
 'fwsc_sha256':variants['etch']['sha256'],'firmware_bytes':variants['etch']['size'],
 'application_bytes':len(app),'application_sha256':sha(app),'project_format':'PJT31',
 'native_storage_enabled':True,'ota_enabled':True,'variants':variants,
 'verified_original_stock_reference_sha256':sha(stock),
 'trusted_app_only_ota_loader':True,'v145_fx_source_preserved':True,
 'physical_cross_firmware_install_tested':False,'arbitrary_custom_installers_guaranteed':False,
 'hardware_recovery_verified':False}
(R/'RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
(R/'release/V1.48-RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
