#!/usr/bin/env python3
"""Reject UAC RAM overlap with loader code, DSP pool, stacks or boot mailbox.
Software-only ELF range audit; it does not establish physical FM1 safety.
"""
from __future__ import annotations
import argparse, json, subprocess
from pathlib import Path
from fm1_elf_sections import elf_sections

SECTIONS = {
    '.ram_text': (0x01C00000, 0x01C04000),
    '.uac_ram': (0x01C04000, 0x01C06000),
    '.data': (0x01C08000, 0x01C20000),
    '.bss': (0x01C08000, 0x01C20000),
    '.pool': (0x01C20000, 0x01C74000),
    '.noinit': (0x01C7C000, 0x01C7F000),
}

def check(elf):
    sections = {v['name']:v for v in elf_sections(elf)}
    failed = []
    result = {}
    for name,(lo,hi) in SECTIONS.items():
        section=sections.get(name)
        if name == '.uac_ram' and (section is None or not section['size']):
            failed.append('.uac_ram missing in USB-audio firmware')
            continue
        if section is None: continue
        base=section['vma']; used=section['size']
        if not lo<=base and used:failed.append(f'{name}: starts outside allocation')
        if base+used>hi:failed.append(f'{name}: out of region {hex(base+used)} > {hex(hi)}')
        result[name]={'start':hex(base),'size':used,'limit':hi-lo,'free':hi-(base+used)}
        print(f'{name:11} {used:7d} / {hi-lo:7d} bytes; end {base+used:#010x}; free {hi-(base+used):5d}')
    for name in ('.ram_text','.uac_ram','.data','.bss','.pool','.noinit'):
        if name not in result:failed.append(f'missing {name}')
    if result.get('.uac_ram',{}).get('size',0) < 2048+4096+184:failed.append('UAC memory too small to hold ring, scope and endpoint packet')
    if failed: raise SystemExit('FAILED SRAM GUARD: '+ '; '.join(failed))
    print('PASS: UAC ring and scope live in dedicated 8 KiB RAMTEXT tail without overlap with protected pools/stacks/mailbox')
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('elf',type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
    output=check(a.elf)
    if a.report:a.report.write_text(json.dumps({'hardware_tested':False,'sections':output},indent=2)+'\n')
