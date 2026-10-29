## V1.78 · First-Boot Storage Ownership Prompt (software-tested candidate)

- On boot, detect `ETCH_STORE_FOREIGN` in the **ETCH target storage banks**, and offer a modal **FORMAT FOR ETCH? YES/NO** screen. NO is selected initially and exits to RAM-only operation without writes; the question returns on a future boot until storage is made compatible.
- YES requires an explicit K1 selection, physical SEL button confirmation, release of SAVE and a continuous **3-second SAVE hold**. HOME cancels at every stage. The user is warned that old sample slots 2 and 3 will be permanently erased.
- Reuse the existing, confirmed bounded storage initialization in `wave_store.c`: only `0xB4000..0xDBFFF` may be erased. Valid legacy ETCH projects or already initialized ETCH storage are never silently replaced; unrelated boot, OTA staging, sample slot 1, other project and recovery areas are preserved. Read errors, boot-loop Safe Mode and USB-controller boot cannot enter the wipe.
- Show **FORMATTING / VERIFYING** while the work is underway, followed by **FORMAT VERIFIED** or a specific failure. The new prompt state is in the existing `.pool` memory region; project format `PJT31`, original `FM-1_1500` identity, protected 16 KiB header, OTA loader, preset manager and all existing synth features are unchanged.
- Host tests verify default NO, button holds and interruptions, no destructive operations before complete confirmation, permitted flash sectors, protection of valid ETCH projects and safe handling of I/O failure; native JieLi builds succeed with/without USB-audio. No FM1 hardware validation is claimed.

## V1.77 · RAM Preset Rescue & Save Feedback

- Added a versioned RAM-only SysEx snapshot interchange (0x60–0x64), CRC-checked 24-byte chunk transfer. The original OTA identity command and update protocol remain unchanged.
- Added complete live-sound snapshot of A/B/C 128-point waveforms, wave effects, global effects, patch and envelope, ORGANIC, LIVING, MOD MATRIX, evolution and harmony settings. This is **not a full multi-project backup**; projects and patterns beyond the active sound are not transferred.
- Added a standalone Web MIDI Project/Preset Manager for live capture and `.etchpreset` download/import with SHA-256 + CRC32. Upload only auditions in RAM. No automatic flash writes.
- Added real SAVE / VERIFYING feedback before blocking project/preset/wave flash work. Existing success displays remain readback-verified; failure messages remain visible instead of pretending success.
- Preserved original storage ownership protection. `STORAGE LOCKED` is not blindly turned into automatic format; `STORAGE FULL` remains an explicit capacity error when the project does not fit.
- No new product ID, no project serializer changes, no boot/OTA flash layout changes, no UI pages removed. New snapshots work **only after V1.77 boots**; they cannot rescue unsaved V1.76 RAM by flashing V1.77.
- Software builds and tests are not physical FM1 verification. Full bidirectional hardware USB-MIDI transfer and storage recovery are still unverified.

## V1.74 — DSP arithmetic optimization & SRAM guard audit (29 October 2026)

- Converted four bounded Q15 one-pole filtering steps from 64-bit to 32-bit signed intermediate multiplication. The coefficient upper bound is 15,000, and the bounded sample/state difference is at most 65,535, making the product at most 983,025,000 (below INT32_MAX). Integer division semantics remain unchanged, as verified by four deterministic V1.73 PCM reference hashes.
- Added a reusable `fx_pcm_v174` host regression and section/symbol linker audit `tools/v174_memory_audit.py`; this rejects SRAM budget growth, unexpected stack-guard addresses or overlapping USB-audio/pool regions on the tested UAC48 target.
- JieLi FM1 builds for no-UAC, USB 44.1 and USB 48 kHz compile; native regressions include FX, audio, polyphony, projects, USB audio, overload and patch swaps. See `docs/V1.74-DSP-MEMORY-REPORT.md` for exact results.
- All 62 native screens, SIMPLE/ADVANCED navigation, STRING ENS, all 278 native source/header files, PJT31 project persistence, calibration, MIDI, audio engine, OTA recovery, original bootheader, loader and original `FM-1_1500` identity are retained.
- **No RAM reduction and no physical CPU improvement has been established.** The DSP pool is still essentially full (4 B free). Stack high-water and full hardware stress tests remain outstanding. This is not an FM1 hardware-qualified release.

## V1.73 — Bounded LCD compositor and rendering efficiency (29 October 2026)

- Replaced the inner loop of the standard 5×7 label renderer with a direct, RGB565 byte-swapped glyph blit **only when fully inside the existing active canvas**. The original rendering method remains as a fallback for partial glyphs, unbuffered screens and scale-2 splash graphics. No changes to fonts, colors or target panel pixels are intended.
- Added preflight guards for offscreen scope/UI height, y-origin, missing begin state, and invalid composite commit before writing to the fixed 240×173 RGB565 canvas or sending LCD DMA. No additional framebuffer and no malloc.
- Verified the new glyph blit against the previous per-pixel fill semantics in 8,192 deterministic randomized canvases under ASan and UBSan. Guard-canaries and invalid rectangle/edge rejection passed.
- Preserved all 62 screen IDs, every existing source C/header file, SIMPLE/ADVANCED catalog, STRING ENS, all MIDI/audio/FX modules, PJT31 storage and native OTA code. Only the native UI composite code and version string changed from V1.72.
- JieLi app builds for normal FM1 and USB audio 44.1/48 kHz, 16/16 targeted native regressions, firmware/installer SHA parity and pinned identity all passed. This is a host/software result, not a physical FM1 measurement.
- **RAM limitation:** `.pool` remains 344,060/344,064 bytes (4 bytes free), `.bss`+`.data` unchanged, UAC pool unchanged. This iteration improves the draw path and bounds safety, **not** DSP memory capacity. No claim of lower physical CPU load without hardware profiling.
- Release provides one official `ETCH-V1.73.fwsc`; a legacy stock-bootheader ETCH variant remains only inside the installer. The official identity is still `FM-1_1500`, never `FM-1_1730`.
- Full 62-screen physical pixel QA, external cross-firmware installs and recovery tests remain open.

## V1.72 — Screen Catalog and String Ensemble (29 October 2026)

- Added an ADVANCED screen catalog covering all **62** existing pages. SIMPLE navigation and all existing physical shortcuts remain intact; no page IDs, synth modules or serializers were removed.
- K3 browses page IDs and K4 opens the selected screen. MONO-only editors are guarded in POLY. NAME, PROJECT SAVE and PRESETS use the existing initialization and confirmation flows.
- Added **STRING ENS** as an additional factory string-machine-inspired preset in slot 17 of fresh, unsaved factory content. The original 16 named starters are byte-for-byte unchanged in definition. Existing stored projects are not migrated or overwritten.
- New non-destructive strings audition from WORKFLOW (K1 twice) for users with existing PJT31 data. Preview uses the existing wave render, envelope and chorus and leaves all persistent data untouched until an explicit SAVE; active notes prevent heavy waveform rebuilds.
- Preserved unchanged OTA package identity `FM-1_1500`, standard bootheader, trusted loader, WebInstaller and PJT31 format.
- Tests: JieLi 44.1/48-kHz USB build; portable factory, catalog, and legacy workflow checks; USB-audio packet/60-second soak, FX and audio checks; installer SHA and UFW checks. Full hardware and whole-UI pixel capture are not yet verified. RAM pool margin remains 4 bytes.

# ETCH · Changelog

Development and release history of **ETCH**, the SikkenWorks custom synthesizer firmware for the M-VAVE FM1.

This file contains product changes, fixes, optimizations and migration notes. For what ETCH is and how to operate it, use [README.md](README.md); the complete key/shortcut reference also lives there. Historical release details are consolidated below instead of scattered across separate Markdown files.

**Important terminology:** “built”, “host-tested”, “installer-validated” and “hardware-tested” are different milestones. Unless explicitly noted, the historical reports document software testing, **not** proof of a successful physical FM1 installation or independent recovery.

## V1.79 · RGB VHS Boot + Flicker-Free Setup — 29 October 2026

- Replaced the old static pre-audio splash with nine bounded RGB chromatic-shift and VHS scan-glitch frames, then an unmodified SIKKENWORKS screen with `BORN OUT OF PASSION.` held for one second (watchdog is fed throughout). Animation runs before the audio engine starts, without extra framebuffers, heap allocations, flash operations or changes to USB/interrupt code.
- Added a first-class boot workflow choice: **ETCH → SIMPLE / ADVANCED**, default SIMPLE, select with K1 and confirm with SEL. This is runtime-only and cannot write to flash. All 62 existing page IDs and the complete ADVANCED catalog remain accessible. The existing foreign-storage YES/NO takeover question is shown *after* workflow selection, not removed.
- Eliminated the previous storage modal's repeated visible full-LCD erase: all changing content now composes into the existing 240×173 RGB565 panel and is transferred only on state changes; an offscreen header avoids a transient black top panel. A quantized dirty-state test verifies no redundant draws for steady input. Preserved the separate hold-to-format guard, NO default, protected flash ranges, recovery priority and all status/error messages.
- Host tests: 16/16 native audio/USB/project/FX regressions; boot dirty-state tests; the original V1.78 format confirmation and flash-boundary integration; installer CHECK/WRITE simulated regression; firmware packages verified to keep exact `FM-1_1500` identity, protected 16 KiB bootheader, unchanged OTA loader and matching installer SHA-256.
- Normal/UAC 44.1/UAC 48 kHz JieLi builds passed; no physical FM1 display timing, real USB performance or power-loss recovery claim. The effect intentionally has colored registration glitches during the animation, **never** on the stable branding or interactive prompts.

## V1.76 · Continuous Analog-Style ORGANIC Drift — 29 October 2026

- **Fixes audible stepping in ORGANIC DRIFT.** Former V1.75 updated random pitch targets every 0.09–0.91 seconds and settled in roughly 12ms, producing abrupt tuning jumps followed by holds. V1.76 uses a bounded quadratic ease-in/out with a flat slope at the endpoints, moving continuously between random destinations over **2.5–9 seconds** depending on RATE.
- New deterministic per-voice state uses the existing `seed`, `time`, `pitch`, and `goal` fields. `time` packs elapsed and duration ticks, with no extra SRAM or heap allocations. Interpolation runs at the existing 32-sample control rate, not per PCM sample; all easing intermediates fit in 32 bits.
- Turning DRIFT to zero on held notes now eases back to neutral over approximately 1.5 seconds instead of snapping. A newly loaded zeroed patch remains a bit-exact bypass. PHASE and per-voice static VARIANCE behavior are retained.
- 2,424 sampled randomized target transitions pass monotonicity, endpoints and maximum control-step checks under sanitizer; the original ORGANIC + audio/FX/project/USB regressions remain passing.
- JieLi native builds for no UAC, 44.1k and 48k UAC compile; 48k linker retains **12,292 bytes free DSP pool** and unchanged boot/stack/OTA regions. The native firmware and packaged `.fwsc` keep identity **`FM-1_1500`** and `PJT31`.
- **Not proven on physical FM1 hardware.** Listen to a sustained single note in MONO with high DRIFT and RATE to confirm smoothness; no new hardware stability/flash safety assertion is made.

## V1.75 · Wave History RAM Recovery — 29 October 2026

- Removed **12,288 bytes of duplicate static undo/redo snapshots** from the runtime-only waveform editor. The previous two 16-entry arrays are now one reversible 16-entry circular history, with exact sample swaps that preserve both the 16-step undo depth and redo after undos. No waveform resolution, tools, limits or functionality is dropped.
- On the linked 48 kHz UAC FM1 build, `.pool` shrinks from **344,060** to **331,772 bytes**, leaving **12,292 free** instead of 4. Ordinary RAM, UAC scratch and protected stack guard locations are unchanged. The change is **static RAM recovery**, not a measured CPU-speed improvement.
- New reference-model stress test compares **25,032 edit/undo/redo operations** to the original V1.74 algorithm on all three waveforms. Added host test integration and a link-time memory regression gate.
- Preserved every native UI page and 62-screen catalog, SIMPLE/ADVANCED, STRING ENS, 16 starter sounds, 8 voices, effects, USB 44.1/48 kHz, project storage `PJT31`, stock/OTA safety handling, package identity **`FM-1_1500`**, protected 16 KiB bootheader and pinned loader.
- JeeLi target builds, native audio/FX/USB/project/OTA software regressions and `.fwsc` SHA-verification passed. **No on-device flashing, USB soak or ROM recovery has been validated**. Do not describe this as an on-device stable release.

## V1.71 · SIMPLE/ADVANCED Workflow + UI Safety — 29 October 2026

- Adds a runtime-only SIMPLE mode (default): SELECT cycles SOUND, MORPH, FILTER, FX, LIBRARY; FX, LFO, ARP and SEQ open their familiar entry pages without forcing long advanced cycles. Existing ADVANCED mode retains the V1.70 routing and all 62 native editor page IDs.
- GLO from SIMPLE opens SYSTEM's new eighth WORKFLOW tab. K2 switches between SIMPLE/ADVANCED. The mode is deliberately not written to PJT31, so opening previous projects never changes content or format.
- Fixes a verified V1.70 layout error: the fourth knob's exclusive right edge was **241**, while label_box rejects edges above **240**. Labels for K4 could therefore disappear entirely. V1.71 bounds K4 at 240.
- Clips all generic footer numeric values to their own parameter boxes, not only their labels; unexpected large numbers are explicitly marked instead of bleeding into adjacent columns. Auxiliary page titles get bounded boxes to avoid overlapping status and fault messages.
- Adds native tests for both workflow modes, 8 system tabs, all 62 page title strings, four parameter cell boundaries, and static 240x240 literal-text bounds. No new frame buffers or audio-thread code; unchanged UAC RAM, DSP pool and project/OTA layout.
- JieLi builds, software regressions and package SHA tests are documented separately. A **physical full 62-screen pixel QA**, audio soak, flash and recovery test have **not been performed**. Only a small, targeted fraction of the screens were geometrically tested.

## V1.70 · Sound-first UI / LCD Layout / Audio-priority Rendering — 29 October 2026

- Redesigned the default PLAY panel as **SOUND**, retaining the large real-time waveform but showing the currently selected preset and only four directly usable sound controls: MORPH, ETCH, TONE (filter cutoff) and SPACE (reverb). Underlying encoder behavior and persisted parameters are unchanged.
- FX now opens a practical four-effect overview on the **first press**, with DRIVE / CHORUS / DELAY / REVERB. Second press and existing advanced FX pages remain available. No effects, modifiers, editor pages or sequencer features were removed.
- Added sixteen named, conservative **factory-only** starter preset recipes. They are seeded into a blank RAM factory bank, not applied as a storage migration and not written over user presets/projects. Existing 16 other patches remain present.
- Added bounded header title boxes and four independent parameter-label boxes. Long captions no longer draw into another knob's allotted region. Simplified on-screen prompts and aligned simulator PLAY visual hierarchy. Static 240x240 main-page rectangle checks pass; exhaustive hardware screenshot parity of all 62 secondary page layouts is not claimed.
- Throttles non-urgent LCD refresh when USB is connected, ISR pressure increases or audio deadlines are missed. Critical fault/page/change notifications repaint immediately; MIDI, DSP, project storage, USB/OTA callbacks are not throttled.
- Native JieLi 44.1 and 48 kHz USB-audio builds and legacy no-UAC build compile; `.uac_ram` and DSP/stack/boot memory boundaries remain unchanged. No new static buffers were introduced; the DSP pool remains nearly full.
- Preserves product/USB identity `FM-1_1500`, app-only OTA/recovery, trusted-loader validation, exact boot header, PJT31 project format, original licenses and source files. One official ETCH-V1.70.fwsc is produced; WebInstaller keeps its isolated legacy-header option.
- Software-only tests are not physical FM1 verification. Long audio/USB soak and all historical UI screens still need on-device testing; do not interpret a successful cross-build as evidence of zero glitches on real hardware.

## V1.61 · USB Audio SRAM Allocation Fix (software-verified candidate) — 22 October 2026

- Fixed the native `--storage --updater --usb-audio` **linker overflow (2,332 bytes)** without reducing any synth buffers, project slots, USB rings, sequencer capacity, FX line sizes, or oscilloscope resolution.
- Added `app-uac.ld`: a USB-audio-only linker variant that partitions the pre-existing 24 KiB Felucca flash-execution RAM window into **16 KiB `.ram_text`** and **8 KiB `.uac_ram`**. The audited default `app.ld` remains byte-identical to the Felucca baseline, and the dedicated 8 KiB lies below reserved debug SRAM, stack, `.noinit` and boot mailboxes.
- Relocated the **USB capture ring + USB IN endpoint TX buffer + optional 48 kHz FIR history + double-buffered 1024-sample scope capture** to this USB-specific scratch region. The scope is explicitly zeroed by `native_start`, and the USB ring is fully primed before publication; no persistent state is expected in the new NOLOAD section.
- Deleted the redundant **1,024-byte USB stereo DMA copy**. `uac_tap` reads the already calculated 24-bit-aligned DAC DMA half and converts Q15<<7 back to original stereo Q15; original Felucca/host tests still use the unshifted input path.
- Added native `check_uac_memory.py` guard for all section limits, loader RAMTEXT, reserved SRAM, full flash app audit and actual ELF placement. Added scaled-DMA 44.1/48 kHz packet, reconnect and clock-drift tests.
- **44.1 kHz + 48 kHz USB builds both compile** with unchanged UI, MIDI, DSP, FX, patch/project format (`PJT31`) and OTA identity **`FM-1_1500`**. The original non-USB V1.60 binary was byte-identical after the memory-only source changes (before updating the visible version to V161).
- 48 kHz is still unverified for physical CPU load, timing and host enumeration; neither an ELF memory audit nor host USB packet tests prove a functioning real FM1 USB audio stream. Keep the known ROM/recovery process available; do not infer physical OTA safety from the build.

## V1.60 · Cross-Firmware OTA Compatibility and Integrity (software-verified candidate) — 22 October 2026

- **Preserved the exact V1.50 product/package and USB identity `FM-1_1500`.** This is intentionally not `FM-1_1600`. V160 is the user-visible release indicator and does not alter package identity or `PJT31` project storage.
- Fixed the V1.50 *incoming* loader-length gate: compatible versioned Felucca-family loaders can have different LZ4 encodings and lengths. The loader container, inner address `0x01C0A800`, decoded `FELUCCA-LOADER-1` tag, bounds and **both compressed-wrapper and uncompressed-body CRCs** are checked in native C.
- Fixed a related staging bug: the **actual staged loader length is always the pinned ETCH writer's 6,898 bytes** for app-only updates. It no longer uses the incoming loader length and cannot read past its trusted embedded binary when another compatible build has a larger OTA container.
- Verified the full UFW `flash.bin`/`ota.bin` CRCs, file offsets and declared sizes before erase. Strengthened application verification to include complete encrypted-file integrity, decoded app-area and `app.bin` CRCs, `isd_config.ini` key `0x980F` and standard JieLi XIP/size/boundaries. Invalid input aborts before writing the OTA region.
- Kept the original protected 16 KiB outside app-only writes; a different unused incoming bootheader no longer causes automatic rejection, while a tampered package still fails its global checksum.
- Continued to stage and SHA-check **only ETCH's original trusted app-only executable** (`ota-FM-1_015`). Untrusted incoming loader code is parsed but never run. Official V15 full-flash restore remains an exact pinned-image exception. USB-MIDI handshake, completion acknowledgement, readback, owner record and recovery logic remain in place.
- Reused existing OTA-suspended scope DMA memory for bounded LZ4 integrity validation, avoiding a permanent 8 KiB RAM increase. No audio, synth, sequencer, UI workflow, samples or project schema were removed or simplified.
- Built a JieLi pi32v2 application and regenerated *both* boothead `.fwsc` variants and the embedded WebInstaller with independently matching SHA-256. Source, tests and files are included; the stock-head ETCH variant is not official stock firmware.
- Host native replay passed on exact ETCH `.fwsc` fixtures; an alternate **synthetically re-encoded** but otherwise equivalent loader passed while malformed, corrupted and unknown-family loaders failed before commit. USB interruption, failed staging I/O and missing host confirmation failed closed. The existing browser regression suite passed.
- **Not hardware-tested:** actual ETCH → original Felucca 1.6/1.1.5 → ETCH, official vendor V15 restoration and physical power-loss/recovery must still be verified on an FM1. A host simulation is not proof of a universally interoperable flashing path.

## V1.50 · Native Cross-Firmware OTA Compatibility (experimental)

- Fixed the **native** outgoing OTA CHECK logic: an app-only firmware update no longer requires its unrelated protected bootheader to match ETCH’s own bootheader. The trusted app-only loader preserves the protected 0x0000–0x3FFF range.
- Replaced the incoming-loader SHA whitelist with a safer handoff for standard app-only updates: ETCH **never executes the other firmware’s loader**, but stages a verified embedded copy of ETCH’s original SHA-256-pinned 6898-byte loader. Incoming app-only loaders must still have valid structural headers, be exactly the expected length and pass their transmitted data CRC.
- Preflights the destination flash image’s filesystem headers, CRCs, `isd_config.ini` chip key `0x980F`, application region `0x4000`, app directory and `app.bin` offset `0x120` before any flash erase. Rejected/invalid targets never reach commit.
- Retains the exact identity `ota-FM-1_015` for its actual staged loader, rather than advertising a Felucca `900` loader which is not the executable being staged. External installers must recognize the actual OTA identity; hardware compatibility still requires testing.
- Preserved the exact-original-V15 full-flash recovery restriction and existing staged-loader readback/CRC/SHA, owner-token, timeout, interruption and flash-commit safety guards. Does not make the official V15 restore image freely interchangeable with custom images.
- Reproduced the historical bootheader and loader-hash failure classes in a **native host harness** and tested foreign-header acceptance, foreign-loader safe substitution, corrupted app directory refusal, timeout and flash-read/erase/write failures. Full original Felucca 1.6 and 1.1.5 packages have **not yet been replayed**; actual device compatibility is unverified.
- Compiled as `FM-1_1500` with both V15 and ETCH bootheader variants. Synth/audio signal path and PJT31 data structures unchanged.

## V1.49 · Resource Efficiency / Matrix Processing

- Combined per-voice Mod Matrix evaluation into a single four-route pass across MORPH, Wave C Depth, Filter Cutoff and optional SPACE, replacing repeated route scans per destination.
- Preserved integer rounding, modulation saturation, per-voice expressions and the first-main-voice SPACE policy. The former public matrix API remains intact.
- Kept 8 voices, three GROUPS parts, original DSP/FX, PJT31 project storage and B1 HF2 cross-firmware installation support.
- Added 500,000 randomized modulation equivalence tests and seven PCM checksum comparisons against the previous version, with a pinned V1.48.5 audio reference.
- Removed redundant historical Markdown files, duplicate legacy installers and superseded release images from the distribution project; original source and license files remain.
- Native build identity `FM-1_1490`; dual-header `.fwsc` packages and embedded installer payloads regenerated from pinned reference boot/OTA assets.
- Physical FM1 real-time CPU savings and installation remain **unverified**; earlier 369% load warning is not claimed resolved by host tests.

## V1.48.5 · New native firmware / B1 HF2 installer

- Recompiled the native FM1 application with identity **`FM-1_1485`**; not just a renamed V1.48 `.fwsc`.
- Produced **two newly packaged firmware variants** containing the same application with different preserved 16 KiB protected-header families: ETCH and original V15.
- Updated the B1 HF2-based WebInstaller to embed both new binaries, verify SHA-256 and look for `FM-1_1485` after reboot.
- Kept project storage and OTA enabled, with **PJT31** and all existing synthesis behaviour unchanged.
- Verified native compilation, package structure and targeted installer/browser tests. Physical cross-firmware flashing and independent recovery remain unverified.

## V1.48.4 · B1 HF2 OTA transition (installer only)

- Adapted the user-supplied **SikkenWorks B1 V4.3 SLOOP Transition Installer HF2** reconnect sequence for ETCH.
- Supported the Felucca/SLOOP `FM-1_900` → `ota-FM-1_9xx` loader family in addition to the known `ota-FM-1_015` loader.
- Kept on-device **CHECK**, loader identity verification and firmware hashing mandatory before WRITE.
- Preserved both existing V1.48 `.fwsc` variants **byte-for-byte**; no changes to the firmware application's audio, UI or storage.
- Tested simulated transition, CHECK refusal, unsupported-loader refusal and embedded firmware equality. Hardware results still pending.

## V1.48.3 · Safer OTA handoff (installer only)

- Adjusted the post-CHECK transition to avoid rapidly opening/closing the previous MIDI port while the device is changing USB modes.
- Added a quiet handoff interval, targeted loader identity polling and a specific diagnosis when the original application (`FM-1_900`, for example) comes back instead of an OTA loader.
- Blocked all WRITE operations unless a recognized OTA identity is established.
- Kept the firmware image unchanged. The fix did not establish a successful real-device handoff.

## V1.48.2 · Legacy ETCH CHECK / header fix (installer only)

- Reproduced a failure from running firmware **`FM-1_1038`**: its historical updater rejected the ETCH-header package (`-5`) after **18** CHECK read requests.
- Confirmed in a simulated historical receiver that the identical ETCH application with the original V15 protected header advanced through **65** CHECK requests.
- Changed AUTO selection for identified historical V1.34–V1.38 ETCH versions to use the original V15-header variant; rejected known contradictory manual choices.
- Added a regression test for the 18-block failure. Both firmware package payloads remained unchanged.

## V1.48.1 · OTA reconnect diagnostics (installer only)

- Increased the loader wait window from 30 to 90 seconds and improved discovery of renamed USB MIDI port pairs.
- Added diagnostic logging and recognized direct reconnect to a known OTA loader.
- Kept device-side CHECK and loader validation intact; a firmware that does not actually launch the loader cannot be forced to do so by the browser.

## V1.48 · Cross-firmware OTA compatibility

- Introduced two firmware packages with identical ETCH application/OTA code and distinct, byte-pinned ETCH versus stock V15 protected headers.
- Introduced package selection based on the running FM1 identity while keeping the firmware's own CHECK mandatory.
- Extended ETCH's own OTA receiver to recognize the **exact validated official V15** restore, including its protected-header difference, without permitting arbitrary images or bootloaders.
- Required pinned loaders and header checks; refused unknown firmware targets.
- Assigned application identity **`FM-1_1048`**. Audio, GROUPS, project serialization and synthesis parameters stayed unchanged from the previous generation.
- Native/package/host compatibility checks passed; compatibility with every outside custom firmware or installer was **not** and cannot be guaranteed.

## V1.47.1 · Cross-firmware recognition (installer only)

- Removed an ETCH-only device identity allowlist that rejected other valid FM1 firmware before they could perform CHECK.
- Added recognition of common third-party FM1 identities and MIDI port names, while still requiring a real FM1 SysEx identity reply.
- Refused unsafe unknown loader identities and preserved SHA-256/header verification.
- Left the native V1.47 application and `.fwsc` unchanged (`FM-1_1047`).

## V1.47 · Display rendering and audio-timing guards

- Reduced redundant LCD/SPI transfers with a four-band screen change cache, avoiding a new full framebuffer.
- Added a full-frame fallback for large changes.
- In simulations, verified **2,000 pixel-identical final frames**; a one-band change reduced pixel transfer from 83,040 to 20,640 bytes and an unchanged screen required no pixel resend.
- Kept native audio source identical to V1.46.2 and passed seven reference PCM comparisons.
- Added host checks around DMA/deadline guard behaviour; no claim of measured physical ISR headroom.

## V1.46.2 · Realtime Mod Matrix optimization

- Cached static per-part Mod Matrix route validity and Wave C requirements **per audio block**, rather than rechecking them for every voice/sample.
- Preserved the original sample-rate modulation arithmetic, shared eight voices, GROUPS, FX and PJT31.
- Verified **7/7 bit-identical** reference PCM scenarios versus the V1.46.1 restored audio path and randomized modulation equivalence tests.
- Recompiled as `FM-1_1462`. The 60 MHz emulator report of **369% audio-thread load** was **not proved fixed** by these tests.

## V1.46.1 · Emergency audio rollback

- **Rolled back the V1.46 FX optimization** after a reported immediate audio-engine crash.
- Restored the complete V1.45 native FX/audio implementation; the compared V1.45 application and recovery application differed only in version bytes in the documented binary comparison.
- Added rollback parity checks and removed the withdrawn V1.46 FX package from the recovery distribution.
- Produced an experimental recovery application and installer, without claiming a physically verified cure.

## V1.46 · Withdrawn FX resource optimization

- Introduced dry/wet FX fast paths that skipped certain chorus, delay and reverb computations when not contributing to output.
- Host PCM and selected FX-state comparisons passed, **but a physical user reported the audio engine stopping immediately**.
- **Withdrawn**. Do not use the original V1.46 FX package as a known-stable release. It was replaced by V1.46.1 rollback and later guarded optimizations.

## V1.45 · Native resource optimization

- Moved repeated GROUPS preset lookups and Wave FX branch checks from the per-sample path to per-block preparation.
- Replaced certain safe bounded 64-bit oscillator/filter calculations with equivalent 32-bit arithmetic.
- Reported byte-identical PCM in tested mono/poly/GROUPS/FX scenarios and no added persistent DSP buffers.
- Did **not** demonstrate a reliable physical FM1 CPU-saving percentage.

## V1.44 · Performance Mode

- Added PERFORMANCE screen (HOME+ARP), **four assignable macros** and **two parameter scenes (A/B)**.
- Added live/assign modes, capture with REC and recall with PLAY/STOP; saved assignments and scenes in **PJT31**.
- Preserved compatibility when reading older PJT30 projects.
- Changed a small existing sequencer workbuffer placement to fit native RAM without adding large DSP allocations.

## V1.43 · Wave Sequencing and navigation fixes

- Introduced WAVE SEQ, with step-specific A/MIX/B choice, morph amount and step lock, reusing the active patch's Wave A/B and existing project lock storage.
- Replaced unreliable HOME double-tap with **HOME+SAVE** for PROJECTS; added **HOME+FX** for GROUPS.
- Added a chapter-based PROJECT HELP screen navigated with K1, fixed the `CAN T` display typo to `CANT`.
- Added dirty-region drawing on GROUPS to reduce full-screen flicker during knob changes.
- Made **MONO** the default for new projects while preserving the saved mode of existing projects.

## V1.42 · Mod Matrix 2.0

- Added MIDI aftertouch and keytracking as Mod Matrix sources.
- Extended amount control to signed **−100…+100**; retained four routes and four destinations to respect RAM constraints.
- Introduced **PJT30** with backward reading of PJT29 and earlier compatible formats.
- Added updated browser matrix editor and restored missing `.gitignore` / GitHub Actions test workflow.

## V1.41 · Advanced Wave Sculpting

- Added **FOLD, BEND, ASYMMETRY, PHASE DISTORTION and HARMONIC TILT** to a dedicated SCULPT page.
- Added original-wave/preview drawing, wave selection, strength and pivot; SAVE commits the edit to the 128-point waveform and creates an undo checkpoint.
- Preserved the existing waveform save and project formats. Applying is undoable, but **does not create a permanent nondestructive effect stack**.
- Added corresponding simulator controls and tests.

## V1.40 · Expressive Voice and GROUPS (development phases)

- Added per-voice **LINEAR / SOFT / HARD** velocity curves and a SYSTEM page to select them.
- Introduced **PJT28** so velocity response could be stored per preset; historical PJT26/PJT27 loads default to LINEAR.
- Added MIDI channel aftertouch toward morph and brought the web aftertouch audio path into closer alignment with the native engine.
- Fixed a Living Waves regression where the intended per-voice evolution was bypassed in normal polyphonic processing.
- Added **two GROUP slots** with saved preset choices, exclusive note ranges, ON/OFF and range capture using the FM1 keyboard or USB MIDI.
- Added voice ownership, Group routing, independent wavetable banks and Group-specific morph, C modulation, filter, drive, envelope, velocity and waveform insert values within **eight shared voices**.
- Introduced **PJT29** with GROUPS project storage and the first PROJECT HELP screen.
- Retained a **single shared post-mix FX bus**; independent chorus/delay/reverb processors per Group were not added.
- Shipped staged source, web and installer test candidates; hardware qualification was not completed by host tests.

## V1.39 · Audio Engine 3.0 development phases

- Added click-reducing DRIVE parameter smoothing and revised the DRIVE saturation path.
- Added overflow guards and bounded arithmetic, plus host tests for eight voices, effects, headroom and extreme input levels.
- Continued anti-aliasing and stability investigations. Host results did not establish complete physical audio performance.

## V1.38 · OTA / stock restore work

- Updated the OTA-CHECK policy around stock V15 restoration and archived reference firmware handling.
- Physical restoration compatibility and an independent ROM-level recovery path remained unverified.

## V1.37 · USB MIDI keyboard boot mode

- Added **hold SEQ at power-on** for silent USB MIDI KEYBOARD MODE using all 27 FM1 keyboard keys.
- Added octave shifting (OCT−/OCT+), note-off handling, USB reconnect and output queue protection.
- Kept normal synth boot, REC Safe Mode and OCT− loader entry separate.

## V1.36 · RIFF / polyphonic stability follow-up

- Fixed ARP mode 7 **NEW RIFF** display truncation and ensured generation changes the stored 16-step motif.
- Allowed a newly generated playing riff to restart promptly from step zero without reentering the ARP editor.
- Continued POLY audio deadline and advanced-DSP isolation work; physical `AUDIO STOP` reports motivated further safety work.

## V1.35 · POLY mode DSP safeguards

- Limited the POLY LFO page loop to MOD, WAVE C FX and MOD MATRIX, bypassing certain advanced mono-oriented DSP modules.
- Preserved their saved settings for a later return to MONO and added graduated voice-budget fallback under missed audio deadlines.
- These changes were safety mitigations, not proof of stable eight-voice operation on every FM1.

## V1.34 · Voice-mode and RIFF refinements

- Continued differentiating advanced controls in MONO versus POLY and refined per-patch RIFF behaviour and navigation.
- Historical development reports include updates to the installer and firmware packaging; see the archived manuals for exact scope.

## V1.33 · POLY performance and mode-aware screens

- Separated heavy mono-oriented advanced processing from POLY paths and improved the on-device navigation for supported audio modes.
- Investigated deadline overruns and audio stops under concurrent notes.

## V1.32 · Audio engine and spectral stability

- Investigated stability with polyphony, spectral processing and advanced per-note controls; added related safeguards and regressions.

## V1.31 · Living, Spectral and Patch Blend fixes

- Improved spectral control isolation, Living Waves playback and safety around the Patch Blend state.
- Built accompanying firmware/installer and native/browser test candidates.

## V1.30 · Advanced controls and preset state

- Expanded advanced edit controls, preset-level sound state and BPM overlay behaviour.

## V1.29 · Advanced control fixes / startup isolation

- Corrected selected advanced controls and continued startup-pop troubleshooting and audio output gating.

## V1.28 · Patch Blend and RIFF 2.0

- Added/expanded PATCH BLEND and RIFF generation, with supporting project and control adjustments.

## V1.27 · Startup pop hardening

- Delayed analog output enable until USB configuration remained stable and added an additional settling delay.
- Preserved a silent PCM output during early startup and documented remaining possible analog transients.

## V1.26 · Original Mod Matrix

- Added a lightweight **four-route** matrix with VELOCITY, ENVELOPE, WAVE C and LIVING sources, plus MORPH, C DEPTH, FILTER and SPACE destinations.
- Stored routes alongside project sound settings with 0…100 positive depth.

## V1.25 · Living Waves

- Added per-note evolving waveform character, LFO navigation and related parameter controls.

## V1.24 · Spectral Etch / RIFF tempo

- Introduced harmonic-level SPECTRAL editing with waveform bake and undo/redo.
- Aligned RIFF tempo/rate behaviour with the shared BPM and MIDI-clock mechanism.
- Corrected an earlier firmware identity mismatch between installer and native app.

## V1.23 · Organic Engine

- Added lightweight, per-voice pitch DRIFT, RATE, PHASE and VARY settings with bypass at zero and low-rate parameter update.

## V1.22 · SPACE 2.0

- Added an all-pass diffusion stage, reverb damping and interpolated stereo chorus reads in the global effects path.
- Preserved the existing FX routing and dry behaviour.

## V1.21 · Audio Engine 2.0

- Expanded band-limited waveform handling, polyphonic audio stability and accompanying host sound/DSP tests.

## V1.20 · Documented core synthesizer

- Documented waveform DRAW with 128-point edits, A/B morph, Wave C modulation, library/preset/project saving, waveform/insert/global FX, arpeggiator, up-to-256-step sequencing, harmony tools, Motion Recording and USB MIDI.
- Documented early storage and UI workflows. Some V1.20 button gestures were superseded in V1.43 and later.

## Earlier ETCH development

- ETCH originated as SikkenWorks' custom M-VAVE FM1 firmware built around **DRAW · MORPH · MODULATE**, moving from the concept and web simulator through initial native JieLi builds, audio stability, projects, presets, OTA packaging and recovery research.
- Earlier iteration-by-iteration records are not exhaustively present in the supplied V1.48.5 archive. No undocumented version-specific changes are inferred here. Historical summary and current instructions are maintained in this CHANGELOG and [README.md](README.md), respectively.

---

### Release integrity and support policy

- Firmware identity alone does not prove that a device can flash, boot or restore that firmware.
- A compatible `.fwsc`, host PCM parity and WebInstaller simulation are **not** substitutes for a physical FM1 test.
- A previous custom firmware may reject CHECK or use a different OTA loader. Never bypass loader checks or blind-flash an unknown target.
- Keep project backups, validated recovery tools and a known-good firmware before upgrading.
- Original third-party notices and license information are documented in [README.md](README.md#credits-and-licensing) and the bundled license folders.
