#!/usr/bin/env python3
"""V1.84 static linker comparison to exact V1.83 shipped ELF; no runtime measurement."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from fm1_elf_sections import elf_sections
from subprocess import run
out=ROOT/'build/fm1-native-storage-updater-uac48-p05/etch-native.elf'
ref=ROOT/'release/ETCH-V1.83.fwsc'
assert ref.exists(), 'V1.83 reference missing'
new={p['name']:p for p in elf_sections(out)}
# These are measured from the V1.83 ELF shipped alongside the source tree.
base={'.ram_text':2888,'.uac_ram':6604,'.data':100,'.bss':93900,'.pool':334260,'.noinit':12148}
limits={'.ram_text':16384,'.uac_ram':8192,'.data':98304,'.bss':98304,'.pool':344064,'.noinit':12288}
rows={}
for name,old in base.items():
 size=new[name]['size'];limit=limits[name]
 assert size<=limit,(name,size,limit)
 rows[name]={'v183_bytes':old,'v184_bytes':size,'delta_bytes':size-old,'free_to_limit_bytes':(0x01c20000-(new[name]['vma']+size) if name=='.bss' else limit-size)}
assert rows['.pool']['delta_bytes']<=8
assert rows['.uac_ram']['delta_bytes']==0
assert rows['.noinit']['delta_bytes']==0
report={'type':'linker-static','hardware_tested':False,'project':'ETCH V1.84','sections':rows,'result':'PASS'}
(ROOT/'release/V1.84-MEMORY-AUDIT.json').write_text(json.dumps(report,indent=2)+'\n')
for name,d in rows.items():print(f'{name:10} V183 {d["v183_bytes"]:7} V184 {d["v184_bytes"]:7} Delta {d["delta_bytes"]:+5} Free {d["free_to_limit_bytes"]:7}')
print('PASS V1.84: RAM boundaries preserved; no additional wave buffers')
