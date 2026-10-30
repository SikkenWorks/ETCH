#!/usr/bin/env python3
"""Read-only regression: stable SIKKENWORKS hold contains no decorative line."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'firmware/fm1-native/bringup.c').read_text()
start=s.index('static void etch_boot_brand_frame(')
end=s.index('static void etch_boot_splash(', start)
frame=s[start:end]
assert frame.count('label("BORN OUT OF PASSION.",40u,145u,')==2
assert 'lcd_fill(40u,163u,160u,1u,0x4208)' not in frame
clean=frame[frame.index('}else{')+7:]
assert 'label_large("SIKKENWORKS",32u,99u,0xffff);' in clean
assert 'label("BORN OUT OF PASSION.",40u,145u,0xffff);' in clean
assert 'lcd_fill(' not in clean
assert '#define ETCH_UI_VERSION "V180"' in s
assert 'fm1_wdt_feed();fm1_delay_ms(25u);' in s
assert 'ETCH_BOOT_BRAND_HOLD_MS' in s
print('PASS V1.80 branding: no underline, unchanged slogan, hold and watchdog feed')
