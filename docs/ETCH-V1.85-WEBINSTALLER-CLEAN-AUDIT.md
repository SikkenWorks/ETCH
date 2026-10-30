# ETCH V1.85 – Clean WebInstaller audit

**Scope:** visual and copy update to the existing V1.85 WebInstaller; **not** a new firmware build.

## Changed

- Removed the three prominent yellow/amber warning cards from the main installation area.
- Preserved the essential experimental firmware, flash/recovery, cross-firmware CHECK, and boot-header caveats in a neutral expandable **Safety & compatibility** section.
- Kept the required risk acknowledgement visible outside the expandable section, and the main install button still requires it.
- Applied the established ETCH PHOSPHOR colors and simplified the page structure, branding and release copy. Corrected stale V1.48/V1.78 headings and described V1.85's 128-point editing and smooth audio transitions.
- Kept the source-family selection, package SHA-256, OTA VERIFY/CHECK/LOADER/FLASH steps, device/transport error messages and downloadable diagnostic log.
- Consolidated multiple duplicated text-selection/right-click blockers into one JavaScript block, leaving input field editing usable.
- Added a permanent repository design rule in `docs/WEBINSTALLER-UI-STANDARD.md`, a README/CHANGELOG entry and `tools/check_webinstaller_ui.py`.

## Verified

- `python tools/check_webinstaller_ui.py`: **PASS** (no yellow warnings; safety disclosure and visible consent retained; embedded firmware SHA-256/byte parity).
- `node tests/test_v185_installer.mjs`: **PASS** (OTA CHECK precedes loader/write; denied CHECK does not advance; loader identification; both embedded package hashes match original firmware files).
- `python tools/embed_v185_installer.py`: **PASS** (regenerates embedded payloads without overwriting cleaned UI).
- Headless Chromium markup/layout checks: desktop 1440px and mobile 390px viewports; no horizontal overflow, no JS page errors, 0 warning boxes, expandable safety section opens. Previews captured with `set_content` because direct local-file browser navigation was restricted.
- Both embedded `.fwsc` packages remain **byte-identical** to the original V1.85 firmware assets. Primary package SHA-256: `b05d33c5bfa4c082b9989e997c7ece8652d3e1e78cf1dc26b5298393689e5e9e`; stock-header compatibility package SHA-256: `b7943fc6ae76036953812a6fde36b3735343fcf35da58aca7bd86f3b5cf790f4`.
- Firmware product identity unchanged: `FM-1_1500`.

## Not tested

- Real FM1 connected via Web MIDI, actual flashing, device restart or hardware recovery.
- The cosmetic update **does not** prove cross-firmware or stock restoration on physical hardware.
- Browser screenshots made via `set_content` cannot execute the cryptographic package check in a secure Web MIDI browser origin. Package checks were instead run through the native Node.js test harness.

## Future releases

Use the cleaned `WebInstaller.html` as the source template. Update the release copy, firmware bytes and hashes for each new ETCH version, then run the OTA tests **and** `tools/check_webinstaller_ui.py` with its package paths updated. Avoid reintroducing multiple yellow warning cards. Keep the explicit risk checkbox and critical failures visible.
