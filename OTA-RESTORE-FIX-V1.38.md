# ETCH V1.38 - official stock restoration compatibility

## Root cause / GitHub report
The submitted log has a stock V15 firmware package hash `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`, device `FM-1_1027`, and stops at CHECK after reading offsets `0`, `512`, and `0xA5F00` (`ota.bin` header).

The **old firmware** (not the browser UI) pinned *only* the community 6,898-byte update loader and rejected M-VAVE's original 19,937-byte loader before allowing stock installation. This creates OTA-FAIL while retaining the ability to upgrade ETCH -> ETCH. Its 16-KiB protected boot head check is *not* the cause for the authentic stock V15 image: it is byte-identical to V1.37.

## What changed
- `firmware/fm1-native/ota.c`: PINNED dual-loader policy. The unmodified M-VAVE V15 full-flash loader is allowed **only** when the host also sends the exact manufacturer V15 `flash.bin`, verified using its own SHA-256. The ETCH/Felucca app-only loader stays pinned independently. Any unrecognized loader or changed stock image is rejected **before erase**. Protected first 16 KiB must match installed flash on both paths. No arbitrary or generic firmware bypass.
- Stock loader and flash image are separately hashed, and the staged loader is read back after write. Old updater protections, staging bounds, checksums, and fail-closed behavior remain.
- `ETCH-WebInstaller-V1.38-OTA-RESTORE-FIX.html`: installs `FM-1_1038`, verifies exact embedded package SHA-256; accepts only original stock V15 SHA-256 via file picker; recognizes old ETCH versions and **refuses direct stock flash until the on-device V1.38 fix has been installed**. Explicitly tells the user why and what to do.
- `tests/test_v138_restore_gate.py` included in CI to prevent accidentally removing the stock compatibility path in future releases.

## If the device is currently running ETCH V1.37 or older
1. **Do not attempt the stock V15 file directly** on that existing firmware. The browser cannot override the device's OTA-CHECK code.
2. Back up projects and make sure an independently tested full flash recovery method is available.
3. If proceeding with this **unverified experimental** build, use the V1.38 webinstaller to install ETCH V1.38 via the existing ETCH-to-ETCH app-only loader route. Wait for the FM1 to reboot and report `FM-1_1038`.
4. Reopen the V1.38 webinstaller, choose `RESTORE - OFFICIAL V15`, select the **unmodified** original `FM-1.fwsc`. The original V15 file must have SHA-256 `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`.
5. If on-device OTA-CHECK confirms and the ROM loader can complete the write, the FM1 should reboot into `FM-1_015`. Recheck USB MIDI identity and audio, buttons, display, and normal updater detection. **Actual FM1 hardware validation is still required.**

## What this does not do
It cannot fix a device whose firmware/USB update interface cannot run; that needs separately proven ROM-level recovery (e.g., Transporter), not a website. It does not guarantee the official manufacturer's desktop installer will detect ETCH's custom USB name before stock is restored. It does not guarantee preservation of user projects or samples across stock restore.

## Verified offline
- Build: native JieLi pi32v2, 186,116 B app, reproducible SHA-256.
- Update package: 610,086 bytes, `FM-1_1038`; intact copied 16,384-byte stock/V1.37 protected header and unchanged known ETCH community updater component.
- Host-simulated ETCH-to-ETCH CHECK/OTA staging/commit: pass.
- Host-simulated ETCH-to-*exact official V15* CHECK/OTA staging/commit: pass (1,291 requests).
- Corrupted loader, bad package ranges, mismatched boot header, interrupted transfer, and injected NOR read/erase/write faults: refuse to commit.
- Browser: exact embedded binary parity, ETCH/stock hash verification, old-ETCH bridge requirement, altered binary rejection: pass.
- Physical FM1 stock restore: **NOT TESTED**. This package is an experimental test candidate, **not a certified recovery method**.

GPL-3.0-only, ETCH derived update transport from Felucca by Leo Kuroshita / Hügelton Instruments; retain source credits and GPL licenses.
