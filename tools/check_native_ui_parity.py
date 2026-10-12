#!/usr/bin/env python3
"""V0.47 source-level audit of page coverage, controls and safe transitions.
This is NOT LCD pixel testing or physical FM-1 verification.
"""
from pathlib import Path
import re,sys
root=Path(__file__).resolve().parents[1]
native=(root/'firmware/fm1-native/bringup.c').read_text()
nav=(root/'firmware/fm1-native/native_ui_navigation.h').read_text()
sim=(root/'sim/app.mjs').read_text()
checks={
 'four-screen PLAY/MORPH/FILTER/LIBRARY carousel': 'const HOME_PAGES=[\'PLAY\',\'MORPH\',\'FILTER\',\'LIBRARY\']' in sim and 'pages[4]={ETCH_UI_PLAY,ETCH_UI_MORPH,ETCH_UI_FILTER,ETCH_UI_LIBRARY}' in nav,
 '28 defined native page IDs': 'ETCH_UI_PAGE_COUNT=28' in nav and 'etch_ui_title(native_page)' in native,
 'all native page name entries': len(re.findall(r'"[A-Z0-9 ]+"',nav.split('titles[ETCH_UI_PAGE_COUNT]=',1)[1].split('};',1)[0]))==28,
 'fifths page and knob routing': all(x in native for x in ['native_page==26','etch_ui_fifths_move(native_harmony.root,dm)','KNOB 1 ROTATE']),
 'dedicated SYSTEM diagnostics': all(s in native for s in ['SYSTEM USB AUDIO','STORAGE STATUS','RECOVERY STATUS','NO VERIFIED RESTORE']),
 'MOD WAVE C visual': 'WAVE C' in native and 'native_editor.points[2]' in native,
 'MORPH+LIBRARY previews': 'etch_ui_wave_morph' in native and 'etch_ui_wave_load_click' in native,
 'A/B/C EDIT selection': 'etch_ui_draw_target(native_page,native_morph_focus' in native,
 'SEQ and ARP and FX subpages': all(s in native for s in ['etch_ui_seq_next(native_page)', 'etch_ui_arp_next(native_page)', 'etch_ui_fx_next(native_page)']),
 'UI cancellation on page transitions': 'etch_ui_cancel_on_transition(page_before,native_page,draw_before,native_draw' in native,
 'waveform draw/editor and WaveFX': all(s in native for s in ['etch_editor_move(&native_editor','etch_editor_undo(&native_editor)','etch_editor_tool(&native_editor','etch_editor_bake_fx(&native_editor']),
 'named save/patch UI': all(s in native for s in ['native_name_turn(', 'native_patch_save(muted)', 'native_library_save(muted)']),
 'system menu cannot flash': 'NO FLASH FROM MENU' in native and 'if(native_page==25)' in native,
 'UI labels clipped on-device': 'etch_ui_text_capacity(x,y)' in native,
}
for n,passed in checks.items(): print(('PASS ' if passed else 'FAIL ')+n)
print(f'RESULT {sum(checks.values())}/{len(checks)} source-level checks; NOT a native LCD screenshot suite')
sys.exit(0 if all(checks.values()) else 1)
