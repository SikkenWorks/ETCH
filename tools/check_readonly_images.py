#!/usr/bin/env python3
"""Fail-closed symbol check for a read-only ETCH hardware-test ELF.
This checks linked entry points and compile definitions, not instruction-by-
instruction formal verification or what a separate .fwsc installer writes.
"""
import argparse,os,pathlib,re,subprocess,sys,json
ap=argparse.ArgumentParser()
ap.add_argument('--elf',type=pathlib.Path,required=True)
ap.add_argument('--options',type=pathlib.Path,help='native-build-options.json')
ap.add_argument('--stage0',action='store_true')
a=ap.parse_args()
nm=pathlib.Path(os.environ.get('JIELI_TOOLCHAIN',str(pathlib.Path.home()/'.jieli/toolchain')))/'pi32v2/bin/nm'
if not nm.is_file() or not a.elf.is_file():sys.exit('FAIL missing toolchain nm or ELF')
symbols=subprocess.check_output([str(nm),'--defined-only',str(a.elf)],text=True)
# Reject every linked flash-write entry; stage0 additionally rejects all flash
# transport routines, not merely writes. Tests fail if a future version links one.
ban=['fl_erase4k_ram','fl_prog_ram','fl_erase4k','fl_write',
     'ota_session','ota_erase','ota_prog','ota_commit','native_store_erase','native_store_program',
     'fm1_enter_update']
if a.stage0:ban+=['fl_read_ram','fl_jedec_ram','fl_plain_window_init']
present=[s for s in ban if re.search(r'\b'+re.escape(s)+r'\b',symbols)]
if present:sys.exit('FAIL read-only ELF contains flash/update entry points: '+str(present))
if a.options:
 opts=json.loads(a.options.read_text())
 if opts.get('storage_writes') or opts.get('ota_writes') or not opts.get('flash_read_only'):
  sys.exit('FAIL writable build option in image manifest')
print('PASS linked readonly symbol audit',a.elf,'checked',len(ban),'forbidden entries')
