from pathlib import Path
p=Path(__file__).resolve().parents[1]
s=(p/'firmware/fm1-native/bringup.c').read_text()
h=(p/'firmware/fm1-native/native_preset_browser.h').read_text()
assert '#define ETCH_PRESETS_HOLD_MS 1500u' in h
segment=s.split('uint8_t page_before=native_page,draw_before=native_draw;',1)[1].split('   if((pressed&(1u<<6))',1)[0]
assert 'if(preset && !native_draw && native_page==ETCH_UI_PRESETS)' in segment
assert 'ETCH_UI_NAME' not in segment and 'ETCH_UI_PROJECT_SAVE' not in segment
assert 'native_preset_save_slot+' not in segment and 'native_slot+' not in segment
assert '#define FELUCCA_ID "FM-1_1020"' in s
assert '#define ETCH_UI_VERSION "V120"' in s
print('V1.20 preset input routing: PASS')
