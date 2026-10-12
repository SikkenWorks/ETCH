#!/usr/bin/env python3
"""Explicit experimental FM1 development packaging. Never queries or flashes a device.

The release-pack R0 proof gate is unchanged. This tool makes an authorized
development artifact, with provenance/unknown recovery status recorded.
"""
from pathlib import Path
import argparse,hashlib,json,tempfile
import fm1_fwsc as fw
STOCK_SHA='db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a'
LOADER_BODY_SHA='d0b1a05c63030b58e5be06b93751e6cc375d3d34eec584109f97fd31f0a42a78'
def build(app:Path,stock:Path,community:Path,out:Path):
 if out.suffix.lower()!='.fwsc':raise ValueError('Development output must end in .fwsc')
 ok,errors=fw.validate_app_audit(app,Path(str(app)+'.audit.json'))
 if not ok:raise ValueError('App audit: '+'; '.join(errors))
 options=json.loads((app.parent/'native-build-options.json').read_text())
 if options.get('app_sha256')!=hashlib.sha256(app.read_bytes()).hexdigest() or options.get('storage_writes') is not True or options.get('ota_writes') is not True or options.get('stock_only_restore') is not False or options.get('product')!='FM-1_932':raise ValueError('Only audited V0.33 --storage --updater builds accepted')
 appdata=app.read_bytes();errors=fw.validate_app_binary(appdata)
 if errors:raise ValueError('App: '+'; '.join(errors))
 raw=stock.read_bytes()
 if hashlib.sha256(raw).hexdigest()!=STOCK_SHA:raise ValueError('Stock is not the pinned original V15 package')
 parsed=fw.parse_ufw(raw)
 if not(parsed['header_crc_ok'] and parsed['list_crc_ok'] and parsed['chip']=='AC791N' and parsed['declared_total']==parsed['logical_size']):raise ValueError('Stock directory invalid')
 flash_entries=[x for x in parsed['entries'] if x['type']==0 and x['name']=='flash.bin']
 if len(flash_entries)!=1 or not flash_entries[0]['data_crc_ok']:raise ValueError('Stock flash payload invalid')
 original=flash_entries[0]['data']
 if len(original)!=0x93000:raise ValueError('Unexpected original app region')
 ref=fw.extract_stock_assets(community)
 ota=ref['ota']
 if hashlib.sha256(ota[32:]).hexdigest()!=LOADER_BODY_SHA:raise ValueError('Community loader differs from inspected bytes')
 # The official archive has extra encrypted vendor entries unsupported by the
 # community-format validator. It is trusted only by its exact full-file hash;
 # its flash.bin CRC and directory are checked above. Normalize only a private
 # temporary asset-extraction wrapper, retaining original flash bytes.
 with tempfile.TemporaryDirectory(prefix='etch-stock-assets-') as td:
  normalized=Path(td)/'stock-flash-with-reviewed-loader.fwsc.lab'
  normalized.write_bytes(fw.build_fwsc(original,ota,'FM-1_015'))
  assets=fw.extract_stock_assets(normalized)
 image=bytearray(fw.build_flash_image_from_assets(appdata,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload']))
 # Preserve the ENTIRE physical head, including padding and vendor metadata.
 image[:0x4000]=original[:0x4000]
 package=fw.build_fwsc(bytes(image),ota,'FM-1_932')
 with tempfile.TemporaryDirectory(prefix='etch-test-verify-') as td:
  tmp=Path(td)/'test.fwsc';tmp.write_bytes(package);report,errors=fw.verify_fwsc(tmp)
  if errors:raise ValueError('Generated development package: '+'; '.join(errors))
 assert bytes(image[:0x4000])==original[:0x4000]
 out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(package)
 restore=out.parent/'FM1-V15-RESTORE.fwsc';restore.write_bytes(fw.build_fwsc(original,ota,'FM-1_015'))
 rr,ee=fw.verify_fwsc(restore)
 if ee:raise ValueError('Restore package: '+'; '.join(ee))
 (out.parent/'FM1-V15-ORIGINAL.fwsc').write_bytes(raw)
 manifest={'kind':'EXPERIMENTAL_DEVELOPMENT_FIRMWARE','version':'0.33.0','product':'FM-1_932','file':out.name,'sha256':hashlib.sha256(package).hexdigest(),'app_sha256':hashlib.sha256(appdata).hexdigest(),'official_stock_sha256':STOCK_SHA,'stock_head_sha256':hashlib.sha256(original[:0x4000]).hexdigest(),'head_identical_to_stock':True,'loader_body_sha256':LOADER_BODY_SHA,'loader_reference_sha256':hashlib.sha256(community.read_bytes()).hexdigest(),'app_audit_passed':True,'offline_package_verified':True,'hardware_tested':False,'recovery_proven_on_target':False,'release_candidate':False,'persistent_storage_writes':True,'full_specification_complete':False,'native_ota_writes':'reviewed_loader_packages','note':'Requested experimental development artifact. No target recovery proof is asserted. Release gate remains unchanged.'}
 out.with_suffix('.fwsc.dev.json').write_text(json.dumps(manifest,indent=2)+'\n')
 return manifest
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('app',type=Path);ap.add_argument('stock',type=Path);ap.add_argument('community',type=Path);ap.add_argument('out',type=Path)
 ap.add_argument('--acknowledge-development',action='store_true',help='explicitly authorize experimental artifact generation without asserting recovery proof')
 a=ap.parse_args()
 if not a.acknowledge_development:ap.error('Experimental test generation requires --acknowledge-development; this does not authorize device flashing')
 try:print(json.dumps(build(a.app,a.stock,a.community,a.out),indent=2));return 0
 except (ValueError,OSError) as e:print('TEST PACKAGE REFUSED:',e);return 2
if __name__=='__main__':raise SystemExit(main())
