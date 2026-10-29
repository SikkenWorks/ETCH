#!/usr/bin/env python3
"""V1.74 read-only linker audit; checks current allocations and stack reserved map.

This DOES NOT measure stack high-water, heap use or hardware timing.
"""
import argparse, json, re, subprocess
from pathlib import Path
from fm1_elf_sections import elf_sections

# Boundaries from ETCH V1.73 app-uac.ld. No memory map modifications in V1.74.
BOUNDS={
 '.ram_text':(0x01c00000,0x01c04000),
 '.uac_ram':(0x01c04000,0x01c06000),
 '.data':(0x01c08000,0x01c20000),
 '.bss':(0x01c08000,0x01c20000),
 '.pool':(0x01c20000,0x01c74000),
 '.noinit':(0x01c7c000,0x01c7f000),
}
BASELINE={'.ram_text':2888,'.uac_ram':6604,'.data':96,'.bss':94204,
          '.pool':344060,'.noinit':12148}

def audit(elf:Path, toolchain:Path):
 sections={s['name']:s for s in elf_sections(elf)}
 assertions=[]
 report={'hardware_validated':False,'source_release':'1.74',
         'static_only':True,'areas':{},'stack_guards':{},'largest_allocations':[]}
 for name,(start,end) in BOUNDS.items():
  s=sections.get(name)
  if s is None:raise ValueError('missing expected section: '+name)
  if not(start<=s['vma'] and s['vma']+s['size']<=end):
   raise ValueError('out-of-bounds: '+name)
  if s['size']>BASELINE[name]:
   raise ValueError(f'RAM regression in {name}: {s["size"]} > {BASELINE[name]}')
  report['areas'][name]={'vma':hex(s['vma']),'size':s['size'],
   'budget_bytes':end-start,'remaining_bytes':end-(s['vma']+s['size'])}
  print(f'{name:10} {s["size"]:8} / {end-start:8}, free {end-s["vma"]-s["size"]}')
 data_end=sections['.data']['vma']+sections['.data']['size']
 bss=sections['.bss']
 if data_end>bss['vma'] or bss['vma']+bss['size']>0x01c20000:
  raise ValueError('bss/data overlap')
 if sections['.uac_ram']['vma']+sections['.uac_ram']['size']>0x01c06000:
  raise ValueError('UAC RAM touches reserved 8 KiB tail')
 nm=toolchain/'pi32v2/bin/nm'
 symbols=subprocess.check_output([str(nm),'-n',str(elf)],text=True)
 for sym,expected in {'_guard0':0x01c74000,'_ustack_lo':0x01c74100,
   '_ustack_top':0x01c7a000,'_guard1':0x01c7a000,
   '_sstack_lo':0x01c7a100,'_sstack_top':0x01c7c000}.items():
  hits=re.findall(r'^([0-9a-fA-F]+)\s+[A-Za-z]\s+'+re.escape(sym)+r'$',symbols,re.M)
  if len(hits)!=1 or int(hits[0],16)!=expected:
   raise ValueError(f'stack/guard drift {sym} actual={hits}')
  report['stack_guards'][sym]=hex(expected)
 sized=subprocess.check_output([str(nm),'-S','--size-sort',str(elf)],text=True)
 for line in sized.splitlines():
  m=re.fullmatch(r'([0-9a-f]+)\s+([0-9a-f]+)\s+([A-Za-z])\s+(.*)',line)
  if not m:continue
  address,length=int(m[1],16),int(m[2],16)
  if 0x01c20000<=address<0x01c74000 and length>=1024:
   report['largest_allocations'].append({'name':m[4], 'bytes':length,
       'address':hex(address)})
 report['largest_allocations']=sorted(report['largest_allocations'],key=lambda x:-x['bytes'])[:15]
 print('PASS: fixed SRAM partition, code/USB segments, stack guards and RAM non-regression')
 return report

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('elf',type=Path)
 p.add_argument('--toolchain',type=Path,required=True);p.add_argument('--report',type=Path)
 a=p.parse_args();r=audit(a.elf,a.toolchain)
 if a.report:a.report.write_text(json.dumps(r,indent=2)+'\n')
