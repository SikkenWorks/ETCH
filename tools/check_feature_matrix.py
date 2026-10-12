#!/usr/bin/env python3
"""Audit 49 immutable feature IDs, with recorded changes since V0.49.

Only exact, documented transitions may change the original 0.49 statuses.
"""
from pathlib import Path
import json,re
base=Path(__file__).resolve().parents[1]
def old_rows():
 text=(base/'docs/FEATURE-MATRIX-V0.49.md').read_text(encoding='utf8')
 matches=re.findall(r'^\| ([A-Z]\d\d) \| (DONE|PARTIAL|OPEN|BLOCKED) \| (.*?) \|$',text,re.M)
 return {fid:state for fid,state,_ in matches}
ledger=json.loads((base/'docs/FEATURE-LEDGER.json').read_text())
old=old_rows();entries=ledger['features'];new={x['id']:x['status'] for x in entries}
assert ledger['revision']=='0.57',ledger['revision']
assert len(old)==len(new)==len(entries)==49, (len(old),len(new),len(entries))
assert old.keys()==new.keys(), 'Feature IDs added, dropped or renamed'
assert set(new.values()) <= {'DONE','PARTIAL','OPEN','BLOCKED'}
assert all(x.get('description','').strip() for x in entries)
allowed={'H05':('OPEN','DONE'), 'M06':('OPEN','PARTIAL'), 'I03':('OPEN','DONE'), 'P05':('OPEN','DONE'), 'U02':('OPEN','PARTIAL'), 'M05':('OPEN','BLOCKED')}
for fid,after in new.items():
 before=old[fid]
 if before!=after:
  assert (before,after)==allowed.get(fid),f'Unexpected transition for {fid}: {before} -> {after}'
counts={state:list(new.values()).count(state) for state in ('DONE','PARTIAL','OPEN','BLOCKED')}
assert counts=={'DONE':34,'PARTIAL':8,'OPEN':1,'BLOCKED':6},counts
matrix=(base/'docs/FEATURE-MATRIX-V0.57.md').read_text(encoding='utf8')
for fid in new:assert fid in matrix or new[fid]=='DONE',f'Missing tracked ID {fid}'
print('PASS 49 stable feature IDs; H05, M06, I03, P05, U02 and M05 transitions checked; counts:',counts)
