#!/usr/bin/env python3
"""Generate V1.46.2 .fwsc and offline installer from a tested native image.
Hard gates: original V1.45 FX source, seven golden PCM hashes, application
identity, flash partition invariance, OTA bytes, header, container CRC.
No hardware installation is performed and no hardware claim is implied.
"""
from pathlib import Path
import base64,hashlib,json,re,sys,subprocess
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
R=Path(__file__).resolve().parents[1]
APP=R/'build/fm1-native-storage-updater-p05/app.bin'
REF=R/'compat/ETCH-V1.40-OTA-BASE-REFERENCE.fwsc'
OUT=R/'release/ETCH-FM1-V1.46.2-REALTIME-AUDIO-OPTIMIZED.fwsc'
INSTALLER=R/'WebInstaller.html'
IDENTITY='FM-1_1462'
def sha(b):return hashlib.sha256(b).hexdigest()
def replace_one(s,p,r):
 s,n=re.subn(p,lambda _:r,s,count=1,flags=re.S)
 if n!=1:raise RuntimeError('Pattern expected once: '+p)
 return s
def main():
 fx=R/'firmware/fm1-native/native_fx.c'
 if fx.read_bytes()!=(R/'tests/reference/V1.45-native_fx.c').read_bytes():
  raise RuntimeError('BLOCK: withdrawn V1.46 FX change is present')
 subprocess.run([sys.executable,str(R/'tools/test_v1462_pcm.py')],check=True)
 a=APP.read_bytes()
 if b'FM-1_1462' not in a:raise RuntimeError('Wrong compiled app identity')
 if fw.validate_app_binary(a):raise RuntimeError('Invalid FM1 app')
 report,errors=fw.verify_fwsc(REF)
 if errors:raise RuntimeError('Bad known-good OTA reference: '+str(errors))
 assets=fw.extract_stock_assets(REF)
 entries=fw.parse_ufw(REF.read_bytes())['entries']
 oldflash=next(x['data'] for x in entries if x['name']=='flash.bin')
 newflash=fw.build_flash_image_from_assets(a,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
 if len(oldflash)!=len(newflash):raise RuntimeError('Flash length changed')
 newflash=oldflash[:0x4000]+newflash[0x4000:]
 payload=fw.build_fwsc(newflash,assets['ota'],IDENTITY)
 OUT.parent.mkdir(exist_ok=True);OUT.write_bytes(payload)
 checked,errors=fw.verify_fwsc(OUT)
 if errors or checked['product']!=IDENTITY:raise RuntimeError('Bad generated FWSC '+str(errors))
 contents=fw.parse_ufw(payload)['entries']
 actualflash=next(x['data'] for x in contents if x['name']=='flash.bin')
 actualota=next(x['data'] for x in contents if x['name']=='ota.bin')
 if actualflash[:0x4000]!=oldflash[:0x4000] or actualota!=assets['ota']:
  raise RuntimeError('Bootheader or OTA loader changed')
 decr=bytearray(actualflash[fw.APP_AREA_FLASH:]);fw.sfc_xor(decr,0,len(decr),0,assets['key'])
 if bytes(decr[0x120:0x120+len(a)])!=a:
  raise RuntimeError('Packaged firmware differs from compiled app')
 html=INSTALLER.read_text()
 html=html.replace('V1.46.1 EMERGENCY AUDIO RECOVERY','V1.46.2 REALTIME AUDIO OPTIMIZATION')
 html=html.replace('V1.46.1 AUDIO ROLLBACK: removed the V1.46.1 FX changes completely and restored the exact V1.45 native FX engine. Reported physical crash still requires retesting. All eight voices, GROUPS, four performance macros, PJT31 and existing menus remain unchanged. Hardware performance is not yet measured.',
 'V1.46.2 keeps the verified V1.45 FX code and removes repeated Mod Matrix validity scans inside the audio sample loop. Seven PCM stress scenarios are bit-identical to V1.46.1. The previously reported 369% CPU warning is NOT verified as resolved on the real FM1 or third-party emulator.')
 html=re.sub(r'V1\.46\.1','V1.46.2',html)
 html=html.replace('emergency rollback: restored the V1.45 audio code byte-for-byte.','performance optimization: Mod Matrix routing decisions are cached per audio block.')
 html=html.replace('Emergency rollback: restored the V1.45 audio code byte-for-byte. The V1.46 FX optimization was withdrawn after a reported immediate audio-engine crash. Device-level verification is still pending.',
 'Realtime optimization: reuse validated Mod Matrix routes per DMA block. Seven golden PCM comparisons pass. V1.46 FX optimization remains withdrawn. Real FM1 and 60 MHz emulator deadline tests are still pending.')
 html=html.replace('v===1401||v===latest','v===1401||v===1461||v===latest')
 html=replace_one(html,r'(<script id="firmware-embedded" type="application/octet-stream">)\s*.*?(</script>)',
 '<script id="firmware-embedded" type="application/octet-stream">\n'+base64.b64encode(payload).decode()+'\n</script>')
 html=replace_one(html,r"HASH_ET: '[0-9a-f]{64}'",f"HASH_ET: '{sha(payload)}'")
 html=replace_one(html,r"KNOWN_HEAD_HASH: '[0-9a-f]{64}'",f"KNOWN_HEAD_HASH: '{sha(oldflash[:0x4000])}'")
 html=replace_one(html,r"ID_ET: 'FM-1_\d+'",f"ID_ET: '{IDENTITY}'")
 html=replace_one(html,r'ET_LENGTH: \d+',f'ET_LENGTH: {len(payload)}')
 INSTALLER.write_text(html)
 (R/'ETCH-WebInstaller-V1.46.2.html').write_text(html)
 m=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',html,re.S)
 if not m or base64.b64decode(m.group(1),validate=True)!=payload:raise RuntimeError('Installer package mismatch')
 manifest={'version':'1.46.2','firmware_identity':IDENTITY,'firmware_file':str(OUT.relative_to(R)),
  'fwsc_sha256':sha(payload),'firmware_bytes':len(payload),'application_bytes':len(a),'application_sha256':sha(a),
  'original_v145_fx_exact':True,'protected_bootheader_identical':True,'ota_loader_identical':True,
  'project_format':'PJT31','native_storage_enabled':True,'ota_enabled':True,
  'pcm_golden_scenarios':7,'matrix_cache_random_cases':400000,
  'physical_install_tested':False,'hardware_audio_crash_resolved':False,
  'emulator_60mhz_warning_resolved':False}
 for path in [R/'RELEASE-MANIFEST.json', R/'release/V1.46.2-RELEASE-MANIFEST.json']:
  path.write_text(json.dumps(manifest,indent=2)+'\n')
 print(json.dumps(manifest,indent=2))
if __name__=='__main__':main()
