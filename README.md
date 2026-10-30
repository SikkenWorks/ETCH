<div align="center">

# ETCH

**DRAW · MORPH · MODULATE**

*An open-source waveform-sculpting synthesizer for the M-VAVE FM1.*

**SikkenWorks** · Born out of passion.

[Getting started](#getting-started) · [Controls](#controls--shortcuts) · [Sound engine](#sound-engine) · [Saving](#presets-projects--backups) · [Installing](#installation--recovery) · [User manual](USER_MANUAL.md) · [Cheat sheet](CHEAT-SHEET.html)

</div>

---

## What is ETCH?

ETCH turns your FM1 into a hands-on synthesizer built around **waveforms you draw yourself**. Shape Wave A and Wave B, morph between them, use Wave C to modulate the result, and explore a full set of filters, effects and performance tools.

**Firmware:** V1.86 · **M-VAVE FM1** · **Package/USB identity:** `FM-1_1500` · **Project format:** `PJT32` · **License:** GPL-3.0-only.

## Sound engine

```text
Wave A ── [A inserts] ──┐
                        ├─ MORPH ── FILTER / DRIVE ── WAVE AMP ── ADSR ── FX ── OUTPUT
Wave B ── [B inserts] ──┘    ↑
                             │
Wave C ──────────────── PHASE / MORPH / FX MODULATION
```

- **DRAW:** edit all **128 points** of Wave A, B or C directly on the FM1.
- **Three drawing brushes:** **DETAIL** (5-point local brush), **SOFT** (17-point smooth brush), **EDGE** (single-point sharp brush). Select with the **ALGORITHM** encoder in DRAW.
- **MORPH:** blend the *shape* of A and B, rather than merely crossfading two audio channels.
- **WAVE AMP:** use A, B or their morphed wave to control volume at oscillator speed, with adjustable depth. ADSR remains independent.
- **Modulation:** Wave C phase/morph/FX destinations, four-route Mod Matrix, EVOLUTION, ORGANIC analogue-style drift, LIVING, SPECTRAL and PATCH BLEND.
- **Effects:** pre-morph Wave A/B inserts and global chorus, delay, reverb, drive, modulation and dynamics effects. **FX BYPASS** skips supported expensive processing while preserving values.
- **Performance:** up to eight shared voices, ARP/RIFF, up to 256 sequencer steps, harmony tools, GROUPS keyboard splits, four performance macros and two scenes.

**Sound design tip:** use EDGE at lower/mid pitches with the filter open to hear sharp harmonic differences. High notes use band-limited wavetables, so sharp shapes may sound closer together. SOFT gives smoother contours. The editor is polled every 25 ms; audible wavetable updates may trail the cursor briefly because compiling is throttled to protect audio. **Keyboard auditioning does not edit wave points:** brief K1–K4 encoder deltas around physical note-on/off are ignored in DRAW, then normal controls resume while the note is held.

### Three different kinds of saved data

| Save type | What it contains | Capacity |
|---|---|---:|
| **Waveform** | Reusable, named 128-point single-cycle wave | 64 slots |
| **Preset** | Playable sound, its waves and sound settings | 32 slots |
| **Project** | Working state, patterns, GROUPS and performance data | 4 slots |

A computer `.etchpreset` is a **sound snapshot**, not a full project, sample or sequencer backup.

## Getting started

1. Turn down **MASTER** and start ETCH. After the VHS-style SIKKENWORKS intro, choose **SIMPLE** or **ADVANCED** using **K1** and **SEL**.
2. Press **HOME** for **SOUND**. Try **K1 MORPH**, **K2 ETCH**, **K3 TONE** and **K4 SPACE**.
3. Turn **SELECT** to **MORPH**. Use **K1/K2** to select Waves A/B and **K3** to morph between them. **K4** sets Wave C rate; **ALGORITHM** adjusts ETCH depth on this page.
4. Press **EDIT**. Turn **SELECT** to choose A/B/C. Use **K1** for horizontal position and **K2** to raise/lower the waveform, and **ALGORITHM** for the brush. K3/K4 edit harmonics.
5. Return to SOUND with **HOME**. Set FILTER, ENV and FX to taste, then **save the waveform and preset separately** if you want a reusable wave and a saved sound.

### SIMPLE and ADVANCED

| Mode | Focus |
|---|---|
| **SIMPLE** | SOUND, MORPH, FILTER, FX and LIBRARY for quick sound design |
| **ADVANCED** | Full editor navigation and the 64-page WORKFLOW catalog |

To switch: **GLO → SYSTEM / WORKFLOW → K2**. In ADVANCED, the WORKFLOW catalog uses **K3** to select and **K4** to open a permitted page. Some CPU-heavy modulation pages require MONO mode. The header displays `V186S` or `V186A` according to the selected workflow.

## Controls & shortcuts

**K1–K4** always control the parameters displayed on the current page. **SEL** is a button; **SELECT** is a rotary encoder.

| Action | Controls |
|---|---|
| Return to SOUND | Short **HOME** |
| Browse saved presets | Hold **HOME** ~1.5 s, then rotate **PRESETS** |
| Open PROJECTS | **HOME + SAVE** |
| Open GROUPS | **HOME + FX** |
| Open PERFORMANCE | **HOME + ARP** |
| Toggle MONO / POLY | **OCT− + OCT+** together |
| Open FX BYPASS | In ADVANCED, hold **FX** ~700 ms |
| Start a flat waveform | In DRAW, hold **EDIT** ~700 ms; YES/NO, default NO |
| Undo / Redo | In DRAW: **ENV / LFO** |
| Select brush | In DRAW: rotate **ALGORITHM** for DETAIL / SOFT / EDGE |
| Select wave | In DRAW: rotate **SELECT** for A/B/C |
| Change workflow | **GLO → SYSTEM / WORKFLOW**, then **K2** |
| Control BPM | Rotate **PRESETS** outside the preset browser |
| Generate RIFF | ARP mode 7, turn **K4** |
| Restart a responsive AUDIO STOP | Hold **HOME + PLAY/STOP** ~1 s |

**Do not use double-HOME for PROJECTS.** Use HOME + SAVE. The full [user manual](USER_MANUAL.md) contains the screen-by-screen K1–K4 reference and the [interactive cheat sheet](CHEAT-SHEET.html) provides searchable shortcuts.

## Draw, sculpt & play

In **DRAW**, K1 selects one of the 128 waveform points and K2 adjusts it vertically. DETAIL is for precise contours; SOFT produces rounded slopes; EDGE introduces sharp shapes and stronger harmonic contrasts. The editor uses the existing wavetable banks, recompiling only changed waves. Waveform edits still support **16 shared Undo/Redo history snapshots**.

**Flat line:** hold EDIT, choose YES with K1, press SEL. Only the active wave becomes a zero line, and ENV can undo. Selecting NO or HOME cancels without touching it.

**Live effects vs waveform tools:** WFX, SCULPT and tools alter the editable waveform when baked; Wave A/B inserts and global FX process playback without changing those saved points. Use **DRAW → SAVE** for a reusable wave, and normal **SAVE → PRESET** for a playable patch.

**WAVE AMP:** in ADVANCED, use LFO navigation or WORKFLOW to open WAVE AMP. **K1** selects OFF/A/B/MORPH; **K2** sets depth. This is *audio-rate* volume modulation, not a one-shot volume envelope.

**FX BYPASS:** hold FX in ADVANCED. **K1** selects an effect; **K4 or SEL** toggles ON/BYP; **K2** bypasses all; **K3** restores all. Bypass status is temporary in RAM and does not change saved effect settings.

## Sequencing & live performance

- **ARP / RIFF:** eight ARP modes including mode 7 RIFF, a scale-aware 16-step generator. In mode 7, turn K4 for another riff.
- **SEQ:** up to 256 steps and eight note triggers per step; live REC, HOLD, GRID, LOCKS, BAKE and CLOCK editors. Six lock destinations: MORPH, ETCH, cutoff, chorus, delay, reverb.
- **GROUPS:** HOME + FX opens two optional saved-preset keyboard zones alongside MAIN; all parts share the total eight-voice budget and post-mix effects.
- **PERFORMANCE:** HOME + ARP opens four assignable macros and scene A/B capture/recall. Save a project to retain scenes.
- **MOTION:** record a short performance of MORPH, ETCH, cutoff and SPACE controls.
- **USB MIDI:** notes, clock and implemented expression handling. Booting while holding SEQ starts a MIDI-keyboard-only mode, with local synth audio disabled.

## Presets, projects & backups

**To save a preset or project:** press SAVE outside DRAW, use SELECT to choose the kind of save, then **K1 slot**, **K2 name position**, **K3 character** and **K4 backspace**. Confirm with SAVE. Wait for **SAVING / VERIFYING** and a clear success before powering off. If ETCH reports NOT SAVED, the current patch is still RAM-only.

**To save a wave:** while in DRAW, press SAVE and use the same slot/name controls. **To load a saved wave:** LIBRARY → K1 slot → K2 A/B/C destination → REC twice.

**Computer backup:** use the bundled [`ETCH-Project-Preset-Manager.html`](ETCH-Project-Preset-Manager.html) with Chrome/Edge and Web MIDI/SysEx to read the current sound from RAM, download `.etchpreset` or send one to RAM. It does not include entire projects or sequencer patterns and does not automatically write to flash. The current snapshot protocol supports WAVE AMP and `.etchpreset` V1/V2.

### STORAGE LOCKED / foreign data

ETCH protects data belonging to other firmwares. If a first-boot prompt offers **FORMAT FOR ETCH?**, **NO** preserves it but leaves the affected storage locked. **YES**, SEL and an additional **three-second SAVE hold** may format only ETCH's designated region (`0xB4000–0xDBFFF`), destroying any previous data in those sectors. Do not choose YES without a backup of affected data. A valid ETCH project or flash read error should not trigger destructive takeover.

**If your sound is unsaved, do not reboot, format or update the firmware.** Make a RAM-based `.etchpreset` backup first when the running firmware supports it.

## Installation & recovery

- **Current firmware:** [`release/ETCH-V1.86.fwsc`](release/ETCH-V1.86.fwsc), one regular public package, original identity `FM-1_1500`.
- **Installer:** [`WebInstaller.html`](WebInstaller.html), in the clean PHOSPHOR layout. Its critical checks and risk confirmation are still required even though long technical notes are tucked into a neutral expandable section.
- **Required browser:** desktop Chrome/Edge, Web MIDI/SysEx permissions, proper USB data cable and exclusive access to the FM1 MIDI port.
- **Safety:** keep a verified backup, trusted package checksums and an independently tested recovery procedure for your FM1. Device-side package validation, loader checks and post-update verification must succeed. Never bypass a failed CHECK.
- **Cross-firmware OTA:** structurally compatible Felucca-family application-only updates are validated by the installed device-side loader/check path. Official stock restore uses a separately verified official package/loader. A successful USB transfer is **not** proof of guaranteed firmware switching or physical recovery.

**Status:** V1.86 is software-built and host-tested, but long-term USB-audio reliability, listening tests, flash transitions and independent ROM recovery **have not been fully qualified on physical FM1 hardware**. Smooth note-on, wavetable-switch and voice-steal fades reduce some digital clicks; they cannot guarantee elimination of every audible or analogue startup pop.

## Source & development

| Path | Contents |
|---|---|
| [`firmware/fm1-native/`](firmware/fm1-native/) | Native FM1 firmware, audio, LCD, controls, MIDI, flash and OTA |
| [`engine/`](engine/) | Host-side synth engine |
| [`sim/`](sim/) | Browser simulator (not a replacement for native LCD capture) |
| [`WebInstaller.html`](WebInstaller.html) | Current Web MIDI update installer |
| [`ETCH-Project-Preset-Manager.html`](ETCH-Project-Preset-Manager.html) | RAM sound backup/import tool |
| [`release/`](release/) | Firmware packages |
| [`tools/`](tools/) | Build, pack and diagnostic scripts |
| [`tests/`](tests/) | Firmware and DSP regression tests |
| [`USER_MANUAL.md`](USER_MANUAL.md) | Full operating guide |
| [`CHEAT-SHEET.html`](CHEAT-SHEET.html) | Searchable offline quick reference |

Building requires an independently obtained JieLi pi32v2 toolchain, Python 3 and the repository's build scripts. Source tests and a successful compile do **not** prove hardware flash or recovery safety. Follow the appropriate project scripts for the platform; never substitute unrelated stock boot data.

## Credits and licensing

ETCH is an independent **SikkenWorks** project. The waveform-drawing, A/B shape-morphing and Wave C-centred instrument design, plus ETCH-specific synth, editing and performance work, are developed on top of existing FM1 community foundations.

### Core project and directly derived foundation

| Credit | Contribution / relationship | License / source |
|---|---|---|
| **SikkenWorks** | ETCH concept, product design, synthesis workflow, interface and further development | [SikkenWorks / ETCH](https://github.com/SikkenWorks/ETCH) |
| **Leo Kuroshita (@kurogedelic), Hügelton Instruments** | **Felucca**, the crucial FM1 foundation: boot/startup, hardware abstraction/drivers, interrupts, LCD, parts of audio and USB-MIDI/OTA transport. ETCH modifies/adapts portions of this foundation | [Felucca](https://github.com/hugelton/Felucca) · **GPL-3.0-only** |
| **JieLi Technology** | AC79/AC791 hardware platform, pi32v2 toolchain and original vendor SDK boot/configuration material in the FM1 firmware ecosystem; some protected package components are vendor-origin, not original ETCH code | [JieLi AC79 SDK](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK) · applicable vendor/Apache-2.0 notices |
| **M-VAVE** | Original FM1 hardware and stock firmware platform | Original manufacturer; ETCH is **not** affiliated with M-VAVE |
| **SikkenWorks B1 / user-supplied B1 V4.3 HF2 installer** | Reference for the V1.48.4+ transition/reconnect handling on Felucca/SLOOP loader families | B1 HF2 implementation adapted to the ETCH WebInstaller; upstream Felucca notices retained |

**Felucca is not merely an inspiration:** its GPL-covered firmware and updater infrastructure provide substantive foundations in ETCH. Retain its notices and corresponding source when redistributing a modified firmware or installer.

### Additional upstream projects and attribution inventory

The upstream Felucca licensing inventory also records the following authors and projects. **They are credited here for provenance, reference or inherited notices; this list does not claim that every listed synthesis engine or font binary is present or used directly in ETCH.** Consult the source and license folders for actual inclusion.

| Project / author | Upstream contribution or reference | License where documented |
|---|---|---|
| **X0X**, charlesvestal | Felucca browser-emulator design/implementation reference | GPL-3.0 · [fm1-x0x](https://github.com/charlesvestal/fm1-x0x) |
| **CrispyZebra**, Hügelton Instruments | PHASE-engine oscillator provenance in upstream Felucca | GPL-3.0 · [CrispyZebra](https://github.com/hugelton/CrispyZebra) |
| **DaisySP**, Electrosmith, Corp / **Emilie Gillet** | Physical-model DSP in upstream Felucca | MIT · [DaisySP](https://github.com/electro-smith/DaisySP) |
| **Rings**, Emilie Gillet | Sympathetic strings in upstream Felucca | MIT · [eurorack](https://github.com/pichenettes/eurorack) |
| **msfa / Dexed**, Google Inc. / **Pascal Gauthier** | FM synthesis core provenance in upstream Felucca (the msfa component, not necessarily Dexed in full) | Apache-2.0 · [Dexed](https://github.com/asb2m10/dexed) |
| **Fukiai**, Hügelton Instruments | Icon-font provenance in upstream Felucca | MIT · [Fukiai](https://github.com/hugelton/Fukiai) |
| **Inter Tight**, The Inter Project Authors | Typeface used in parts of the upstream UI pipeline | SIL OFL 1.1 · [Inter Tight](https://github.com/rsms/inter-tight) |
| **DotGothic16**, The DotGothic16 Project Authors | Typeface used in the upstream browser emulator | SIL OFL 1.1 · [DotGothic16](https://github.com/fontworks-fonts/DotGothic16) |
| **Versilian Studios / VCSL** | CC0 sample provenance for Felucca's sampler-related engines | CC0 1.0; see upstream Felucca inventory |
| **Tony Gies / klattsch**, plus **Klatt** and **Hillenbrand et al.** | Formant/vocal-synthesis design and data references recorded by Felucca; no klattsch code copied in Felucca according to its notes | Reference credit · [klattsch](https://github.com/tgies/klattsch) |

**License files retained in this project:** [`LICENSE`](LICENSE) (GNU GPL v3), [`LICENSES/FELUCCA-LICENSING.txt`](LICENSES/FELUCCA-LICENSING.txt) (upstream provenance) and [`firmware/fm1-native/LICENSES/`](firmware/fm1-native/LICENSES/) (MIT, Apache-2.0 and SIL OFL notices). These copyright and license notices take precedence over this prose summary. No third-party font files are offered for download by this README.

ETCH is not officially endorsed by **M-VAVE**, **JieLi Technology**, **Hügelton Instruments** or the credited upstream projects.

---

<div align="center">

**ETCH · SikkenWorks**  
*Born out of passion.*

[Source](https://github.com/SikkenWorks/ETCH) · [User manual](USER_MANUAL.md) · [Cheat sheet](CHEAT-SHEET.html) · [License](LICENSE)

</div>
