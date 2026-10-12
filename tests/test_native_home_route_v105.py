#!/usr/bin/env python3
"""Source-to-audio contract: ordinary HOME never issues a forced voice reset."""
from pathlib import Path
import re
s=(Path(__file__).resolve().parents[1]/'firmware/fm1-native/bringup.c').read_text()
assert '#define FELUCCA_ID "FM-1_1005"' in s
assert '#define ETCH_UI_VERSION "V105"' in s
home=re.search(r'if\(pressed&\(1u<<8\)\)\{(.*?)\n   \}\n   uint8_t just_opened_save',s,re.S)
assert home, 'ordinary HOME event branch missing'
for forbidden in ['native_panic=1','native_seq_run=0','native_seq_recording=0','native_stop()','native_pause()','native_start()','fm1_audio_stop()']:
 assert forbidden not in home.group(1), f'HOME must not {forbidden}'
assert 'if(pressed&(1u<<8))native_panic=1' not in s, 'HOME still panics voices after publishing controls'
assert 'if(panel_action==ETCH_PANEL_BEGIN)' in s and 'native_panel_capture=1;native_panic=1;' in s,'explicit panel diagnostics panic was removed'
assert '((1u<<8)|(1u<<12))' in s,'manual HOME+PLAY recovery was removed'
print('PASS HOME routing preserves voices, transport and deliberate safety recovery')
