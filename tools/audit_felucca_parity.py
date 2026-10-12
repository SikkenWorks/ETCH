#!/usr/bin/env python3
"""Read-only direct-file parity audit against a local Felucca source checkout."""
import argparse,hashlib,json,pathlib,sys
p=argparse.ArgumentParser()
p.add_argument('--felucca',type=pathlib.Path,required=True)
p.add_argument('--stage0',type=pathlib.Path)
p.add_argument('--report',type=pathlib.Path)
a=p.parse_args();root=pathlib.Path(__file__).resolve().parents[1]
ours=root/'firmware/fm1-native';upstream=a.felucca/'firmware'
common=sorted((ours/'hal').glob('*'))
common=[p for p in common if p.is_file()]
common.extend([ours/'app.ld',ours/'crt0.S',ours/'lcd.c'])
diffs=[];same=[];expected_changes=[]
for f in common:
 ref=(upstream/'src/lcd.c' if f.name=='lcd.c' else upstream / f.relative_to(ours))
 if not ref.exists():diffs.append([str(f.relative_to(ours)),'missing upstream'])
 elif f.read_bytes()!=ref.read_bytes():
  if str(f.relative_to(ours))=='hal/fm1_flash.h' and all(x in f.read_text() for x in ['#if !ETCH_FLASH_READ_ONLY','#error "Read-only build cannot include persistent storage or OTA writers"','static inline int fl_prog_ram']):
   expected_changes.append(str(f.relative_to(ours)))
  else:diffs.append([str(f.relative_to(ours)),'different bytes'])
 else:same.append(str(f.relative_to(ours)))
# Do not falsely claim usb.c and OTA adapter equivalence.
report={'reference':str(a.felucca),'identical_hardware_and_boot_files':same,
        'different_hardware_and_boot_files':diffs,
        'expected_modified_hal_flash_readonly_gate':expected_changes,
        'usb_c_exact':(ours/'usb.c').read_bytes()==(upstream/'src/usb.c').read_bytes(),
        'ota_c_exact':(ours/'ota.c').read_bytes()==(upstream/'src/ota.c').read_bytes(),
        'recovery_rom_route': 'same HAL fm1_enter_uboot (if fm1_sys.h is identical)',
        'full_app_ota_note':'ETCH OTA protocol derives from Felucca but adds ownership checks, stock-only gate for old build, bounds and SHA256; not byte-identical',
        'hardware_proof':False}
if a.stage0:
 report['stage0_usb_uses_etch_variant']=(a.stage0/'firmware/usb.c').read_bytes()==(ours/'usb.c').read_bytes()
 report['stage0_flash_writer_headers_absent']=not (a.stage0/'firmware/hal/fm1_flash.h').exists()
if a.report:
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
print('FELUCCA PARITY',len(same),'identical,',len(expected_changes),'expected read-only modification; unexpected',len(diffs),'USB exact',report['usb_c_exact'],'OTA exact',report['ota_c_exact'])
if diffs:print('UNEXPECTED HARDWARE DIFFERENCES',diffs)
sys.exit(int(bool(diffs)))
