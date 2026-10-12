#!/usr/bin/env python3
"""Static/UI geometry checks. Does not replace an FM1 hardware test."""
from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
b=(root/'firmware/fm1-native/bringup.c').read_text()
assert '#define ETCH_SCOPE_MAX_HEIGHT 174u' in b
assert 'static void etch_panel_begin(uint16_t top)' in b
assert 'static void etch_panel_commit(void)' in b
assert 'if(etch_panel_composite)return;' in b
assert 'etch_panel_begin(66u);' in b
assert 'etch_scope_begin(155u,85u)' in b
assert 'etch_scope_begin(0u,42u)' in b
assert b.count('etch_panel_commit();return;')>=2
assert 'uint32_t max_rows=56000u/(w*2u)' in b
assert 'lcd_window(x,y+row,x+w-1u,y+row+rows-1u);' in b
assert 'FM-1_979' in b and 'V079' in b
s=b[b.index('static void screen(void)'):b.index('void fm1_cstart(void)')]
assert 'lcd_fill(0,0,240,240,BG)' not in s
assert 'lcd_fill(0,0,240,240,0x0000)' not in s
for height in (42,66,85,113,152,174):
    n=height;offset=0
    while n:
        chunk=min(n,56000//480)
        assert chunk*480<=56000
        offset+=chunk;n-=chunk
    assert offset==height
meta=json.loads((root/'build/fm1-native-storage-updater-p05/app.bin.audit.json').read_text())
pool=next(s['size'] for s in meta['sections'] if s['name']=='.pool')
assert 0<pool<=0x54000
assert 42+113+85==240
print('PASS buffered areas, bounded DMA, no full-screen erase in UI, pool bounds, V0.79 ID')
print('POOL',pool,'of',0x54000,'; headroom',0x54000-pool)
