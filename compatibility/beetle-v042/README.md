# ETCH V1.38 WebInstaller - BEETLE V0.42 compatibility

**What changed:** Explicit support for the detected BEETLE V0.42 identity `FM-1_2042` in the ETCH install path, with the original strict device-side OTA CHECK intact. Direct stock restore from BEETLE remains blocked: the intended bridge is BEETLE -> ETCH V1.38 -> official V15. The bundled ETCH V1.38 `.fwsc` image is byte-identical to the earlier OTA restore fix.

**Safety:** Only the product identity was verified on a device screenshot. The BEETLE V0.42 firmware binary was not supplied. Loader/header compatibility and actual firmware installation therefore remain **unverified**. Never bypass a failed CHECK. On OTA-FAIL export the log and stop. Do not flash without independently proven recovery and a full backup.

**Future releases:** Retain `FM-1_2042` in both `SUPPORTED_BEETLE_IDS` and `requiresRestoreBridge`. Before claiming hardware support, compare the installed BEETLE V0.42 header and loader or perform recoverable physical validation.

**Test:** Run `node tests/test_v042_compat.mjs` (Node.js 18+).
