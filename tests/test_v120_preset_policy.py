from pathlib import Path
import json
p=Path(__file__).resolve().parents[1]
s=(p/'firmware/fm1-native/bringup.c').read_text()
h=(p/'firmware/fm1-native/native_preset_browser.h').read_text()
assert '#define ETCH_PRESETS_HOLD_MS 1500u' in h
segment=s.split('uint8_t page_before=native_page,draw_before=native_draw;',1)[1].split('   if((pressed&(1u<<6))',1)[0]
assert 'if(preset && !native_draw && native_page==ETCH_UI_PRESETS)' in segment
assert 'ETCH_UI_NAME' not in segment and 'ETCH_UI_PROJECT_SAVE' not in segment
assert 'native_preset_save_slot+' not in segment and 'native_slot+' not in segment
# Firmware identity must follow the *current* release, not a hard-coded
# historical value that silently breaks subsequent regression runs.
version=json.loads((p/'RELEASE-MANIFEST.json').read_text())['firmware_identity']
assert f'#define FELUCCA_ID "{version}"' in s
assert f'#define ETCH_UI_VERSION "V{int(version.split("_")[1])-900:03d}"' in s
print('Presets HOLD/encoder policy and current release identity: PASS')
