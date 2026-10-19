> **V1.48.5 NEW NATIVE FIRMWARE:** New FM1 application compiled as `FM-1_1485`. Both new `.fwsc` variants and the updated B1 HF2 WebInstaller are included. The synthesizer DSP remains unchanged; physical flash acceptance still requires testing. See [V1.48.5 release notes](V1.48.5-RELEASE-NOTES.md).

> **V1.48.4 installer update:** ETCH now incorporates the working B1 HF2 SLOOP/Felucca USB-MIDI transition routine for `FM-1_900` to `ota-FM-1_9xx`. Both V1.48 `.fwsc` variants remain unchanged. See [V1.48.4 release notes](V1.48.4-B1-HF2-OTA-RELEASE-NOTES.md). Physical ETCH installation is still unverified.

> **V1.48.3 installer-only:** A safe OTA handoff waits before touching MIDI ports and distinguishes a still-running FM1 from `ota-FM-1_015`. See [V1.48.3-OTA-HANDOFF-FIX.md](V1.48.3-OTA-HANDOFF-FIX.md). Physical installation remains unverified.

# ETCH V1.48.2: critical bootheader selection fix

**FM-1_1038 is NOT an ETCH-header device.** The original V1.34–V1.38
packages in the archived project use the original official V15 bootheader.
V1.48.1 AUTO incorrectly chose the ETCH header for these revisions.
The exact V1.38 updater rejects that variant (`-5`) after **18** SysEx read
requests; it accepts the V15-head variant after **65** and preserves the
installed bootheader. V1.48.2 defaults to the correct known header and
rejects manual contradictions to the archived references.

This is an **installer-only** change; both V1.48 `.fwsc` files are **unaltered**,
firmware identity remains `FM-1_1048`, and all audio, groups, sequencer, UI,
project storage, OTA firmware receiver and websimulator source remain the same.
A successful hardware flash is not yet confirmed. External custom firmware
may implement different update rules, so universal acceptance is not guaranteed.

Use `WebInstaller.html` and leave the header selector on **AUTO** with
`FM-1_1038`. No firmware data is written if CHECK is refused. Save the log and
check the FM1 screen for the actual error status on any failure.

Run `npm test` for browser regressions and `npm run test:ota1482` for the
historical native V1.38 OTA replay (requires C compiler and Python 3).

---

# ETCH V1.48.1 · OTA Loader Reconnect Fix

**Installer update only.** The V1.48 `.fwsc` packages are unchanged. Waiting for an OTA loader now supports longer USB reconnection (90 s), renamed MIDI ports, and detailed logs. This does not override any on-device CHECK or guarantee an OTA loader will boot. See `V1.48.1-OTA-RECONNECT.md`.

## V1.48 original firmware release


**SikkenWorks · M-VAVE FM1 custom synthesizer firmware**

ETCH V1.48 fixes two real compatibility restrictions found in V1.47.1. The old WebInstaller always transmitted one ETCH-specific 16-KiB protected boot header. Also, ETCH's on-device OTA checker rejected the original M-VAVE V15 package when its known genuine header differed from ETCH's. Both cases now have strictly verified handling.

## Release files

| File | Purpose |
|---|---|
| `WebInstaller.html` | Browser OTA installer with two embedded ETCH variants, per-file SHA-256 validation and FM1 handshake |
| `release/ETCH-FM1-V1.48-ETCH-HEAD.fwsc` | ETCH app with the existing ETCH boot header (common ETCH upgrade route) |
| `release/ETCH-FM1-V1.48-STOCK-HEAD.fwsc` | Identical ETCH app but the exact original M-VAVE V15 protected header (stock and other common FM1 sources) |
| `simulator.html` / `sim/index.html` | Browser simulator (no DSP changes) |
| `firmware/fm1-native/` | Full current FM1 native firmware source |
| `engine/`, `sim/`, `tests/`, `tools/`, `.github/` | Host engine, web, tests, build tooling, GitHub CI |
| `RELEASE-MANIFEST.json` | Cryptographic fingerprints, both package variants, release status |

Both ETCH `.fwsc` variants contain **the same native application and app-only OTA loader**. They differ only in the 16-KiB protected boot header carried for CHECK compatibility. The app-only community loader does not overwrite that header. No new audio algorithms, effects or storage formats were introduced. `PJT31`, eight shared voices, multitimbral GROUPS, SCULPT, MOD MATRIX, PERFORMANCE, MIDI, WAVES, FX, PROJECT HELP and sequencing remain intact.

## Using the WebInstaller

1. Back up your projects with the firmware that is currently installed.
2. Open `WebInstaller.html` in desktop Chrome or Edge and choose **ETCH**.
3. Click **CONNECT** and confirm the detected FM1 identity.
4. The installer selects the legacy ETCH header for recognized ETCH firmware or the stock V15 header for stock and other recognized FM1 firmware families. You can explicitly select either verified package variant in the dropdown.
5. Read the warnings, confirm, then install. The **running firmware must approve CHECK**, and the expected `ota-FM-1_015` loader must identify itself before WRITE starts.
6. Only a final `FM-1_1048` identity after reboot counts as successful installation. If CHECK fails, **stop**. Export the diagnostic log; do not blindly keep retrying.

**Recognized names are not a guarantee of compatibility.** Some unrelated/custom firmware has a different boot header, its own update loader, or an installer that refuses foreign files. No ETCH update can unilaterally change those existing programs. Use an appropriate bridge through genuine V15 where supported, or the source firmware's own update procedure. Device-side checks are not bypassed. A stalled or broken boot may require hardware recovery.

## Official stock restore

The ETCH V1.48 OTA receiver recognizes the **exact original V15** flash and **exact approved official OTA loader** using pinned hashes and may accept the known V15 protected header difference. It does **not** permit arbitrary full-flash or bootloader packages. The stock `.fwsc` is **not bundled**; users must supply their original, unmodified official V15 file.

Expected original V15 SHA-256:
`db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`

## Rebuild from source

The externally supplied official V15 file is required to reproduce its pinned header; the original licensed vendor firmware is not bundled. The JieLi pi32v2 toolchain is also an external dependency.

```bash
export JIELI_TOOLCHAIN=/path/to/jieli/toolchain
python3 firmware/fm1-native/build.py --storage --updater
python3 tools/build_v148_release.py --stock-v15 /path/to/original/FM-1.fwsc
npm test
python3 tools/run_native_tests.py --only audio,fx,poly,voice_steal,midi,store,project,v140_pjt28,performance,matrix_v126,patch_swap
```

For additional pinned stock restore simulation, compile `tests/test_native_v148_cross.c` and supply the original V15 file as its third argument. CI runs the two app-only variants without redistributing original vendor firmware.

## Status and limitations

- **Native pi32v2 build:** built successfully, persistent storage and OTA enabled.
- **Two OTA containers:** verified with the FM1 format validator and embedded package SHA-256 checks.
- **Host receiver tests:** same-header, cross-header, unknown-head rejection, and original V15 restore all pass with a simulated flash.
- **Native/audio/browser regression:** selected tests pass. DSP and project serialization source remains byte-identical to V1.47.1.
- **Physical FM1 installation from arbitrary firmware:** **not confirmed**. No vendor/third-party updater can be forced to accept arbitrary images.
- **ROM-level rescue:** not proven on the user's actual FM1. Do not equate a valid `.fwsc` with guaranteed recovery.

See `V1.48-RELEASE-NOTES.md` and `V1.48-TEST-REPORT.md` for detailed results.
