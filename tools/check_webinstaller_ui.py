#!/usr/bin/env python3
"""ETCH installer UI policy test: keep the clean PHOSPHOR layout across releases.

Usage: python tools/check_webinstaller_ui.py [path/to/WebInstaller.html]
Checks visual/structural rules, never accesses an FM1 or changes firmware.
"""
import base64
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
html_path = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / 'WebInstaller.html'
html = html_path.read_text(encoding='utf-8')

required = {
    'compatibility details': 'id="installation-info"',
    'visible recovery acknowledgement': 'id="risk"',
    'installer connect button': 'id="connect"',
    'installer install button': 'id="install"',
    'midi port chooser': 'id="midi-choice"',
    'package hash display': 'id="firmware-hash"',
    'progress meter': 'id="bar"',
    'diagnostic log': 'id="log"',
    'firmware payload': 'id="firmware-embedded"',
    'compat firmware payload': 'id="firmware-embedded-stockhead"',
    'firmware identity': "ID_ET: 'FM-1_1500'",
    'explicit check safety': 'No failed CHECK is bypassed',
}
for description, phrase in required.items():
    assert phrase in html, f'Missing {description}: {phrase}'
assert '<div class="notice"' not in html, 'Old colored warning boxes returned'
assert '.notice{' not in html, 'Old yellow warning panel styling returned'
assert 'class="installer-details"' in html, 'Safety details must remain accessible'
assert re.search(r'<details\s+class="installer-details"\s+id="installation-info"', html), 'Collapsible safety section is required'
assert html.count('id="etch-interaction-lock"') == 1, 'One JS interaction lock only'
assert 'V1.48 CROSS-FIRMWARE OTA' not in html, 'Stale header content'
assert 'V1.85 UI ALIGNMENT + PHOSPHOR' not in html, 'Stale package title'
assert 'UNVERIFIED LAB FIRMWARE. THIS IS NOT A CERTIFIED RELEASE.' not in html, 'Loud warning banner must not return'
assert 'data-kind=error' in html, 'Critical error feedback must remain distinct'
assert 'id="monitor"' in html and 'id="save-log"' in html, 'Diagnostics must remain available'
assert 'The installer does not back up user data.' in html, 'Important backup warning may not be dropped'

# Validate the embedded firmware still matches the committed release packages.
for script_id, rel in (
    ('firmware-embedded', 'release/ETCH-V1.86.fwsc'),
    ('firmware-embedded-stockhead', 'compat/ETCH-V1.86-LEGACY-STOCK-HEAD.fwsc'),
):
    m = re.search(r'<script\s+id="' + script_id + r'"\s+type="application/octet-stream">\s*([^<]*?)\s*</script>', html, re.S)
    assert m, 'Missing payload: ' + script_id
    binary = base64.b64decode(re.sub(r'\s+', '', m.group(1)), validate=True)
    assert binary == (ROOT / rel).read_bytes(), 'Embedded firmware differs from ' + rel
    assert hashlib.sha256(binary).hexdigest() in html, 'Missing firmware SHA-256'
print('PASS: clean WebInstaller UI, visible consent, collapsible safety, no yellow warning cards, package SHA/byte parity')
