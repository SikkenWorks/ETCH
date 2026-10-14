# ETCH V1.38 - OTA / Stock Restore Fix

**Status: native-built, host-tested experimental candidate. Physical FM1 restoration unverified.**

## Fixed
- Stock restore `OTA-FAIL` at CHECK when running ETCH (confirmed older firmware defect).
- Firmware now accepts the **exact** M-VAVE V15 update loader **only when paired with the exact V15 stock flash image**.
- ETCH's existing community update loader remains supported and SHA-pinned for updates between ETCH versions.
- Preserved 16 KiB protected boot header equality check and OTA/flash CRC and post-write verification. No generic untrusted loader allowance.
- New V1.38 webinstaller detects ETCH versions predating this fix and blocks futile direct-stock attempts. It guides the user to first update ETCH on the device to V1.38.
- Added a permanent regression policy test in CI and a source patch applicable to the already-created V1.39 tree.

## Stock restoration procedure (only after verified backup and recovery)
1. On ETCH V1.37 or earlier, install the **V1.38 ETCH firmware first**, using this V1.38 installer, and wait for `FM-1_1038` to be detected.
2. Reconnect the device and select **RESTORE / OFFICIAL V15** in the same webinstaller.
3. Supply the untouched official M-VAVE `FM-1.fwsc`. The installer requires SHA-256 `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`.
4. Only proceed with verified hardware recovery available, because physical stock restoration has **not yet been exercised on a real FM1**.
5. After a successful stock reboot, confirm identity `FM-1_015`, screen, MIDI, audio, keys, presets and normal manufacturer's updater detection.

## Test results
- AC791N/pi32v2 native build: PASS. Application: 186,116 bytes.
- Repeated native builds produce same SHA-256: PASS.
- Release package: `FM-1_1038`, 610,086 bytes. OTA-loader and protected first 16 KiB same as working V1.37 package: PASS.
- Test official stock file `FM-1_015` from original 2026-07-30 reference: correct SHA-256: PASS.
- Native OTA host tests: both community ETCH and official V15 loaders staging/commit: PASS (65 and 1,291 requests respectively).
- Failure-injection tests: changed loader, changed protected header, corrupt package, missing host, SPI flash read/write/erase failure: NO false commits.
- Webinstaller embedded binary parity and hash gates: PASS.
- Physical FM1 test: **NOT RUN**. No claim of safe guaranteed installation.

## Diagnosis and compatibility
The attached user's log shows `FM-1_1027`, a verified stock file `FM-1_015`, then offsets `0`, `512` and `679680 (0xA5F00)`, where the stock `ota.bin` begins. Old ETCH refuses the V15 loader before writing. A browser-only workaround cannot defeat the old firmware's CHECK policy.

This fix does **not** restore a device that cannot boot or enumerate for USB-MIDI, nor does it promise preservation of data during a return to stock. On an inaccessible device, separately proven ROM-level recovery is necessary.

GPL-3.0-only; preserves Felucca / Hügelton Instruments authorship and credits for derived OTA transport and HAL code.
