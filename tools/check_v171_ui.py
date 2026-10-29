#!/usr/bin/env python3
"""V1.71 host-only UI layout and navigation audit. Does not prove physical LCD parity."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
ui=(root/'firmware/fm1-native/bringup.c').read_text()
nav=(root/'firmware/fm1-native/native_ui_navigation.h').read_text()
for needle in (
 'FELUCCA_ID "FM-1_1500"','ETCH_UI_VERSION "V171"',
 'static uint8_t native_workflow=ETCH_WORKFLOW_SIMPLE',
 'native_page=etch_ui_workflow_fx(native_page,native_workflow)',
 'native_page=etch_ui_workflow_lfo(native_page,native_mono_mode,native_workflow)',
 'native_page=etch_ui_workflow_seq(native_page,native_workflow)',
 'native_page=etch_ui_workflow_glo(native_page,native_workflow,native_guide_performance)',
 'etch_ui_home_workflow_turn(native_page',
 'if(native_system_tab==7u && dd)native_workflow=etch_ui_workflow_change',
 'label_box(etch_ui_short_title(native_page),12,68,0xFFFF,108u)',
 'etch_ui_cell_number(values[i],bounded_value',
 'label_box(bounded_value,knob_x,214',
 'K2: SELECT VIEW','ADV: ALL 62 EDITORS',
 ):
 assert needle in ui,needle
assert 'ETCH_UI_PAGE_COUNT=62' in nav
assert 'ETCH_UI_SYSTEM_PAGE_COUNT=63' not in nav
# Unbounded fixed-string labels must not draw outside the actual 240x240 panel.
escapes=[]
for m in re.finditer(r'\blabel\("([^"\\]*)",\s*(\d+)(?:u)?,\s*(\d+)(?:u)?,',ui):
 phrase,x,y=m.groups()
 if int(x)+len(phrase)*8>240 or int(y)+7>240:
  escapes.append((phrase,int(x),int(y)))
assert not escapes,escapes
with tempfile.TemporaryDirectory() as temp:
 binary=str(Path(temp)/'test_v171')
 subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',
   str(root/'tests/test_native_workflow_v171.c'),'-o',binary],check=True)
 subprocess.run([binary],check=True)
print('PASS V1.71 host-only: retained pages and identity, bounded titles/values, workflow contract, literal LCD bounds')
