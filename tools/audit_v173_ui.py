#!/usr/bin/env python3
"""Static 240x240 literal glyph and 62-screen navigation guard.
Cannot prove visual non-overlap for runtime strings or dynamic states.
"""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'firmware/fm1-native/bringup.c').read_text()
nav=(root/'firmware/fm1-native/native_ui_navigation.h').read_text()
assert '#define ETCH_UI_VERSION "V173"' in s
assert '#define FELUCCA_ID "FM-1_1500"' in s
assert 'ETCH_UI_PAGE_COUNT=62' in nav
assert 'etch_ui_glyph_fast_565(' in s
assert 'etch_ui_canvas_rect_valid(' in s
assert 'etch_ui_panel_top_valid(' in s
pattern=r'\blabel\("([^"\\]*)"\s*,\s*(\d+)u?\s*,\s*(\d+)u?\s*,'
fixed=list(re.finditer(pattern,s))
fail=[]
for m in fixed:
 label,x,y=m.group(1),int(m.group(2)),int(m.group(3))
 if x+len(label)*8>240 or y+7>240:
  fail.append((label,x,y))
assert not fail,fail
# Every 62-screen route must have a stable ID and title. Invalid IDs refused.
with tempfile.TemporaryDirectory(prefix='etch173-ui-') as d:
 exe=Path(d)/'catalog'
 subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror',
    str(root/'tests/test_native_workflow_v172.c'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print(f'PASS V1.73 static LCD: {len(fixed)} literal labels within 240x240, 62/62 routes, compositor guards')
print('LIMITATION dynamic names, contextual overlaps and physical LCD pixels are not covered')
