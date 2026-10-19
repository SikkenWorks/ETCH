# ETCH · Changelog

Development and release history of **ETCH**, the SikkenWorks custom synthesizer firmware for the M-VAVE FM1.

This file contains product changes, fixes, optimizations and migration notes. For what ETCH is and how to operate it, use [README.md](README.md); the complete key/shortcut reference also lives there. Historical release details are consolidated below instead of scattered across separate Markdown files.

**Important terminology:** “built”, “host-tested”, “installer-validated” and “hardware-tested” are different milestones. Unless explicitly noted, the historical reports document software testing, **not** proof of a successful physical FM1 installation or independent recovery.

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
