#!/usr/bin/env python3
"""Fail closed on V1.70 native SOUND layout/identity regressions.

Host-only source/layout audit; not a real LCD screenshot or FM1 performance test.
"""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1]
src=(root/'firmware/fm1-native/bringup.c').read_text()
factory=(root/'firmware/fm1-native/native_factory.c').read_text()
nav=(root/'firmware/fm1-native/native_ui_navigation.h').read_text()
assert '#define FELUCCA_ID "FM-1_1500"' in src
assert '#define ETCH_UI_VERSION "V171"' in src
assert 'ETCH_UI_PAGE_COUNT=62' in nav
for needle in ('label_box(title,88,14,WHITE,194u)', 'label_box(names[i],knob_x,198',
               'SIMPLE  SEL: NEXT  GLO: MODE', 'SOUND', 'native_page=etch_ui_workflow_fx(native_page,native_workflow)',
               'etch_ui_refresh_budget(', 'native_project_cfg', 'native_patch_change('):
    assert needle in src, f'missing native UI or retained route: {needle}'
for needle in ('WARM KEYS','GLASS PAD','DUST BASS','ETCH_PATCH_SLOTS'):
    assert needle in factory
with tempfile.TemporaryDirectory() as temp:
    out=Path(temp)/'layout_test'
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',
                    str(root/'tests/test_native_ui_v170.c'),'-o',str(out)],check=True)
    subprocess.run([str(out)],check=True)
print('PASS: 62 native pages retained, V1.71 workflow, full product ID, focused geometry')
