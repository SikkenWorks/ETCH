#!/usr/bin/env python3
"""ETCH installer UI policy test: keep the clean PHOSPHOR layout across releases.

Usage: python tools/check_webinstaller_ui.py [path/to/WebInstaller.html]
Checks visual/structural rules, never accesses an FM1 or changes firmware.
"""
import base64
import json
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
html_path = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / 'WebInstaller.html'
html = html_path.read_text(encoding='utf-8')
manifest = json.loads((ROOT / 'RELEASE-MANIFEST.json').read_text(encoding='utf-8'))
release = manifest['release_version']
release_label = f'V{release}'
version_compact = 'V' + release.replace('.', '')

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

# The UI and OTA diagnostics must describe the release actually embedded, not
# only the package bytes. A stale V1.85 banner previously slipped through.
assert f'SIKKENWORKS · {release_label} · FM-1_1500' in html, 'Installer header shows a stale release'
assert f'<b>ETCH {release_label}</b>' in html, 'Firmware selection shows a stale release'
assert f'id="firmware-name">ETCH {release_label}' in html, 'Firmware metadata shows a stale release'
assert f"type==='etch'?'ETCH {release_label}" in html, 'Dynamic selection shows a stale release'
assert f'ETCH {release_label} bootheader compatibility diagnostics' in html, 'Diagnostic log shows a stale release'
assert f'Verify {version_compact} on the FM1 screen' in html, 'OTA completion instruction shows a stale version'
assert 'V1.85' not in html, 'Old V1.85 release text remains in the installer'
assert 'V181' not in html, 'Old V181 screen-verification instruction remains'
assert 'USB identity alone does not verify the installed version' in html, 'Avoid interpreting unchanged FM-1_1500 identity as firmware version verification'
assert 'UNVERIFIED LAB FIRMWARE. THIS IS NOT A CERTIFIED RELEASE.' not in html, 'Loud warning banner must not return'
assert 'data-kind=error' in html, 'Critical error feedback must remain distinct'
assert 'id="monitor"' in html and 'id="save-log"' in html, 'Diagnostics must remain available'
assert 'The installer does not back up user data.' in html, 'Important backup warning may not be dropped'

# Validate the embedded firmware still matches the committed release packages.
for script_id, rel in (
    ('firmware-embedded', manifest['packages']['etch']['file']),
    ('firmware-embedded-stockhead', manifest['packages']['stock']['file']),
):
    m = re.search(r'<script\s+id="' + script_id + r'"\s+type="application/octet-stream">\s*([^<]*?)\s*</script>', html, re.S)
    assert m, 'Missing payload: ' + script_id
    binary = base64.b64decode(re.sub(r'\s+', '', m.group(1)), validate=True)
    assert binary == (ROOT / rel).read_bytes(), 'Embedded firmware differs from ' + rel
    assert hashlib.sha256(binary).hexdigest() in html, 'Missing firmware SHA-256'
print(f'PASS: clean WebInstaller {release_label} labels + V{release.replace(".","")} display instructions, visible consent, safety, package SHA/byte parity')
