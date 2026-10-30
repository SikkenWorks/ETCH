#!/usr/bin/env python3
"""Static linkage assertions for ETCH V1.87 DRAW scheduler integration."""
from pathlib import Path
src=Path(__file__).resolve().parents[1]/'firmware/fm1-native/bringup.c'
s=src.read_text()
loop=s[s.index('/* V1.87: draw the latest cursor/wave points') : s.index('native_last_tripped=native_tripped;',s.index('/* V1.87: draw the latest cursor/wave points'))]
assert 'etch_draw_compile_due(' in loop
assert 'native_compile_edit();last_wave_compile_ms=etch_ms;' in loop
assert s.index('native_parameter_publish();') < s.index('/* V1.87: draw the latest cursor/wave points')
last_render=s.rfind('screen();last_ui_paint_ms=etch_ms;',0,s.index('/* V1.87: draw the latest cursor/wave points'))
assert last_render != -1
assert last_render < s.index('native_compile_edit();last_wave_compile_ms=etch_ms;')
assert 'ETCH_DRAW_POLL_MS:100u' in s
assert '#include "native_draw_key_guard.h"' in s
assert '#include "native_draw_scheduler.h"' in s
assert '#define ETCH_UI_VERSION "V187"' in s
print('PASS: DRAW cursor painted before compile; V1.86 key guard retained, V187 display and adaptive scheduler wired')
