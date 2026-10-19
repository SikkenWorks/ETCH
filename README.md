<div align="center">

# ETCH

**DRAW · MORPH · MODULATE**

*Open-source synthesizer firmware for the M-VAVE FM1*

**SikkenWorks** · Born out of passion.

[**Getting started**](#getting-started) · [**Controls**](#controls-and-shortcuts) · [**Sound engine**](#how-etch-works) · [**Firmware installation**](#installation-and-recovery) · [**Credits**](#credits-and-licensing) · [**Changelog**](CHANGELOG.md)

</div>

---

## What is ETCH?

ETCH turns the **M-VAVE FM1** into a synthesizer built around waveforms you create yourself. Instead of selecting a fixed FM algorithm, you draw a waveform, shape a second waveform, morph between them, and use a third waveform as a modulation source.

The entire instrument is designed for hands-on sound design, using the FM1's own screen, encoders and keyboard. It also supports external USB MIDI, saved sounds, pattern creation, effects, keyboard splits and live performance controls.

**Current project:** V1.50 · Device identity `FM-1_1500` · Project format `PJT31` · GNU GPL-3.0-only.

> **Hardware status:** V1.50 cross-firmware outgoing OTA compatibility is verified in software tests, but direct Felucca 1.6/1.1.5 `.fwsc` package replay and real FM1 hardware transitions are **not yet verified**. V1.50 is an experimental firmware build. Native compilation and installer checks do not establish that a particular FM1 can safely install, boot or recover it. Save your data and use a tested recovery method before flashing. Cross-firmware installation requires cooperation from the firmware already on the FM1.

## How ETCH works

```text
           DRAW / LOAD WAVE A ─┐
                               ├── A/B MORPH ──┐
           DRAW / LOAD WAVE B ─┘                │
                                                ├── FILTER / AMP ── FX ── OUTPUT
           DRAW / LOAD WAVE C ─── MODULATION ──┘

    ENVELOPES · MOD MATRIX · LIVING WAVES · SEQUENCER · MIDI
```

**Wave A and Wave B** form the main playable sound. At 0% MORPH you hear A; at 100% you hear B; positions between create intermediate wave shapes. **Wave C** modifies that sound through its selected modulation route. All three can be drawn or loaded from the library.

A waveform, a preset and a project are different things:

| You save | Purpose | Available slots |
|---|---|---:|
| **Waveform** | A named, reusable single-cycle wave | 64 |
| **Preset** | Wave choices and sound parameters | 32 |
| **Project** | Working set, patterns, GROUPS, macros and scenes | 4 |

Save a newly edited **waveform** to the library if you want to reuse it. Save a **preset** to keep a playable sound, and save a **project** to retain the wider working state. The latest project format is PJT31; older compatible projects can be read, but older firmware may not read projects saved in the new format.

### The synthesizer

| Area | What it does |
|---|---|
| **DRAW / LIBRARY** | Draw and edit 128-point waves; reuse named stored waveforms |
| **MORPH / MOD WAVE** | Blend Wave A/B and apply Wave C modulation |
| **SCULPT / WAVE FX** | Fold, bend, asymmetry, phase distortion, harmonic tilt and other waveform edits; preview before applying |
| **FILTER / ENVELOPE** | Filter type, cutoff, resonance, drive, amp ADSR |
| **MOD MATRIX** | Four routes from velocity, envelope, Wave C, Living, aftertouch or keytracking to morph, C depth, filter or SPACE; signed amount |
| **LIVING / ORGANIC / EVOLUTION** | Evolving per-note character, phase/pitch variation and moving waveforms (availability depends on MONO/POLY mode) |
| **FX** | Chorus, delay, reverb and additional filtering, modulation and distortion effects |
| **ARP / RIFF LAB** | Arpeggiator and scale-aware 16-step riff generator |
| **SEQ / WAVE SEQ** | Step sequencing, note editing, parameter locks and A/B morph choices per step |
| **HARMONY** | Key, scale, chord tools, Circle of Fifths and note guidance |
| **MOTION** | Record and replay movements of the four PLAY parameters |
| **GROUPS** | One MAIN sound plus up to two additional saved presets in non-overlapping keyboard ranges |
| **PERFORMANCE** | Four assignable macros and two scenes (A/B) saved with the project |

### Multitimbral GROUPS

GROUPS divides the keyboard into **MAIN**, **GROUP 1** and **GROUP 2**. Each enabled Group uses its chosen saved preset inside its assigned inclusive key range. Outside those ranges, MAIN continues to play. The end note must be higher than the start note, and enabled Groups cannot overlap.

The three parts share **eight voices total**, not eight voices each. Each Group can use its own A/B/C waveforms, morph, envelopes and important sound parameters; **post-mix chorus, delay and reverb remain shared**. When GROUPS is enabled, the synth may bypass the ordinary mono restriction so the splits can play together.

### Voices and musical control

- New projects default to **MONO**. Press **OCT− + OCT+** together to toggle MONO/POLY. An existing saved project's voice mode is retained when loaded.
- In **POLY**, the LFO cycle is deliberately limited to MOD, WAVE C FX and MOD MATRIX for realtime headroom. In **MONO**, the other advanced pages are available.
- USB MIDI supports musical notes and implemented expression/clock functions. MIDI channel aftertouch can contribute to morph; actual behaviour depends on a connected MIDI controller that sends it.
- The sequencer supports patterns up to **256 steps**, up to eight note triggers per step and six step-lock targets. WAVE SEQ currently switches among **Wave A / A+B mix / Wave B** from the active patch; it does **not** select arbitrary library slots per step.

## Getting started

1. Turn down the output volume before experimenting with new waves or DRIVE.
2. Press **EDIT** to enter DRAW. Use **SELECT** to choose Wave A, B or C.
3. Use **K1** to move along the waveform and **K2** to change its shape. Use **K3/K4** to select and adjust harmonics.
4. Press **HOME**, open **MORPH** with SELECT and blend A and B. Open MOD WAVE to bring in Wave C.
5. Adjust the envelope, filter and effects; play the FM1 keyboard or a USB MIDI controller.
6. **SAVE** the waveform, preset or project using the appropriate save context.

The physical display is the authority for context-dependent controls. The tables below describe the implemented V1.50 layout, not the older concept mockups.

## Controls and shortcuts

### Physical controls

| Control | Action |
|---|---|
| **K1–K4** | Four page-specific parameters; labels change by screen |
| **SELECT encoder** | Browse main pages, select wave A/B/C in DRAW, or change contextual tabs/steps |
| **ALGORITHM encoder** | Contextual amount, scene selection or sequencer inspector navigation |
| **PRESETS encoder** | Browse and load presets **inside the PRESETS browser only** |
| **MASTER** | Physical master volume |
| **HOME (tap)** | Return to PLAY / exit the current editor |
| **HOME (hold ~1.5 s)** | Open the named PRESETS browser |
| **EDIT** | Enter / leave waveform DRAW |
| **ENV** | Envelope page; **Undo in DRAW** |
| **LFO** | Cycle modulation pages; **Redo in DRAW** |
| **FX** | Effects pages; WFX/SCULPT pages while drawing |
| **ARP** | Cycle arpeggiator, ARP EDIT, CLOCK and RIFF LAB |
| **SEQ** | Cycle SEQ, NOTES, HARMONY and WAVE SEQ |
| **GLO** | Cycle GUIDE, FIFTHS and SYSTEM |
| **SAVE** | Save flow; applies a preview in applicable edit screens |
| **REC** | Contextual record/confirm function; used in several editors |
| **PLAY/STOP** | Sequencer transport or contextual playback |
| **SEL button** | Confirm/open a selected project or contextual selection (different from the SELECT encoder) |
| **OCT− / OCT+** | Adjust keyboard octave |

### Shortcuts: normal use

| Action | Shortcut / path |
|---|---|
| Return to PLAY | **HOME** |
| PRESETS browser | **Hold HOME ~1.5 seconds**, then use **PRESETS** encoder |
| PROJECTS | **HOME + SAVE** together |
| Multitimbral GROUPS | **HOME + FX** together |
| PERFORMANCE | **HOME + ARP** together |
| PROJECT HELP | **PROJECTS → REC**; turn **K1** through chapters, **REC** to close |
| Toggle MONO / POLY | **OCT− + OCT+** together |
| DRAW | **EDIT** |
| DRAW Undo / Redo | **ENV / LFO** while drawing |
| SCULPT | **EDIT → FX → FX → FX** (WFX1 → WFX2 → SCULPT) |
| MOD MATRIX | Press **LFO** until MATRIX (MONO has more intermediate pages) |
| PATCH BLEND | In MONO, **LFO** once more after MATRIX |
| WAVE SEQ | **SEQ** four times starting outside the SEQ pages |
| RIFF LAB | Press **ARP** repeatedly until RIFF LAB |
| Sequencer inspectors | In SEQ context, rotate **ALGORITHM** for GRID / HOLD / LOCKS / BAKE / CLOCK |
| Velocity curve | **GLO → SYSTEM**, SELECT to tab 7, **K2** for LINEAR / SOFT / HARD |
| MOTION | **GLO → SYSTEM**, tab 1, then **REC** |
| New RIFF | ARP mode **7**, turn **K4** (also in RIFF LAB) |
| Toggle Evolution audition depth | **REC** on EVOLUTION |
| Restart audio after AUDIO STOP | Hold **HOME + PLAY/STOP ~1 second**, if the firmware remains responsive |

**HOME double-tap is no longer the PROJECTS shortcut.** Use HOME + SAVE. A combo does not intentionally activate the standalone action of the second key.

### Startup shortcuts

| Hold while switching on | Function |
|---|---|
| **SEQ** | USB MIDI KEYBOARD MODE (FM1's 27 keys act as MIDI controller; no on-device synth audio in that mode) |
| **REC** | Silent, read-only **application Safe Mode** |
| **OCT−** | Request the chip/UBOOT loader entry path, where supported |

These are different modes. Neither application Safe Mode nor a responding USB port proves independent ROM-level recovery. In MIDI KEYBOARD MODE, OCT−/OCT+ shift the transmitted keyboard octave; restart without holding SEQ to leave it.

### K1–K4 screen reference

| Screen | K1 | K2 | K3 | K4 |
|---|---|---|---|---|
| **PLAY** | MORPH | ETCH / C depth | CUTOFF | SPACE |
| **DRAW** | X position | Y brush | Partial 2–16 | Harmonic gain |
| **MORPH** | Wave A | Wave B | Morph | Wave C rate |
| **MOD WAVE** | Wave C | Depth | Rate | Phase / morph target |
| **FILTER** | Type | Cutoff | Resonance | Drive |
| **ENV** | Attack | Decay | Sustain | Release |
| **WAVE A/B FX** | Slot | Type | Depth | Mix |
| **EVOLUTION** | Depth | Rate | Shape | Sync |
| **SCULPT** | FOLD / BEND / ASYM / PHASE / HARM | Depth | Pivot | Wave A/B/C |
| **MOD MATRIX** | Route 1–4 | Source | Destination | Signed amount −100…+100 |
| **GROUPS** | Group 1/2 | Preset | ON/OFF | Capture START then END |
| **PERFORMANCE: LIVE** | Macro 1 | Macro 2 | Macro 3 | Macro 4 |
| **PERFORMANCE: ASSIGN** | Target 1 | Target 2 | Target 3 | Target 4 |
| **WAVE SEQ** | Step | A / MIX / B | Morph % | Lock ON/OFF |
| **ARP / RIFF** | Mode | Rate | Octaves | BPM / generate RIFF in mode 7 |
| **ARP EDIT** | Step | Note index | Rest | Length |
| **SEQ** | Length | Rate | Swing | Gate |
| **NOTES** | Voice / note | Velocity | Note length | Pitch |
| **LOCKS** | Destination | Value | Enable | Step |
| **HARM** | Key/root | Scale | Keylock | Chord |
| **CLOCK** | Follow | Gate | Swing | Tempo |
| **MOTION** | Morph | ETCH | Cutoff | Space |
| **SAVE name editor** | Slot | Character position | Character | Backspace |

### Editing and storing

| Task | Procedure |
|---|---|
| **Save a waveform** | DRAW → SAVE; K1 slot, K2 name position, K3 character, K4 backspace; SAVE to confirm/overwrite when prompted |
| **Undo / redo sculpting** | Apply sculpt with SAVE (one editor history checkpoint), then ENV / LFO in DRAW for undo / redo |
| **Load a library wave** | LIBRARY → select slot and destination → **REC twice** to confirm |
| **Set up GROUPS** | HOME+FX → K1 group, K2 preset, K3 ON; K4 then press the low and high keys on FM1 or USB MIDI; save PROJECT |
| **Use PERFORMANCE scenes** | HOME+ARP → SELECT switches LIVE/ASSIGN; ALGORITHM selects A/B; REC captures; PLAY/STOP recalls; save PROJECT |
| **Edit sequencer** | SEQ → SELECT step; when stopped, FM1 keys toggle notes; REC controls live recording |
| **Clear a GRID step** | GRID → **REC twice** to arm then confirm, if allowed; destructive |
| **Delete a project** | PROJECTS → guarded **REC + SAVE** repeated to confirm; destructive |
| **Reset foreign ETCH storage** | SYSTEM/STORAGE → **FX + SAVE held ~5 s**; destructive to affected sectors; only after verified backup |

### Important page cycles

```text
SELECT home:  PLAY → MORPH → FILTER → LIBRARY → PLAY
GLO:          GUIDE → FIFTHS → SYSTEM → GUIDE
ARP:          ARP → ARP EDIT → CLOCK → RIFF LAB → ARP
SEQ:          SEQ → NOTES → HARMONY → WAVE SEQ → SEQ
LFO (MONO):   MOD → WAVE C FX → EVOLUTION → ORGANIC
              → SPECTRAL → LIVING → MATRIX → PATCH BLEND → MOD
LFO (POLY):   MOD → WAVE C FX → MATRIX → MOD
DRAW + FX:    WFX 1 → WFX 2 → SCULPT → editor tools
```

## Working with sounds and projects

**Waveform library.** Draw or process a waveform, then save it in one of 64 slots. WFX can modify the editable shape; SCULPT displays an original/preview comparison before you apply a change. Applying an edit changes the in-memory shape and can be undone; you must still save the wave to retain it.

**Presets.** Hold HOME, browse saved presets with the PRESETS encoder and load one. Saving a preset does not automatically create a new standalone wave-library entry.

**GROUPS.** Prepare saved presets first. The MAIN preset plays outside enabled Group ranges. The group range endpoints are inclusive. Overlapping ranges or an END note below or equal to START are rejected. Save your project to preserve GROUPS.

**Projects.** HOME+SAVE opens the PROJECTS browser. Project files include stored performance/sequence information, including GROUPS, macro assignments and both scenes. PROJECT HELP lives inside this menu and K1 scrolls through the chapters.

**Mod Matrix.** On MATRIX, choose a route, source, destination and bipolar amount. Available sources: OFF, VELOCITY, ENVELOPE, WAVE C, LIVING, AFTERTOUCH, KEYTRACK. Available destinations: MORPH, C DEPTH, FILTER, SPACE. There are four routes, not eight.

**Wave Sequencing.** Use SEQ → NOTES → HARMONY → WAVE SEQ to choose whether a step uses Wave A, a morph between A and B or Wave B. Morph-lock state is stored with the project; these are **not** independent library-wave selections per step.

**Performance Mode.** Four macros can target MORPH, Wave C depth, CUTOFF, DRIVE or SPACE. Two scenes store their parameter values. Scene recall uses the existing parameter smoothing; it is not an automatic continuous A-to-B scene morph.

All documented controls, shortcuts, modes and primary workflow are described in this README. For version-by-version development, fixes and releases, see [CHANGELOG.md](CHANGELOG.md).

## Installation and recovery

**Use the current [`WebInstaller.html`](WebInstaller.html), not an archived installer.** V1.50 includes two `.fwsc` variants in [`release/`](release/): an ETCH-header variant and an original V15-header variant. They contain the same newly compiled ETCH application, identified as `FM-1_1500`; the installer chooses a compatible candidate according to the firmware it detects.

1. Back up projects and preserve your verified, working recovery firmware/procedure.
2. Open the WebInstaller in desktop **Chrome or Edge** over HTTPS/localhost. Use a USB data cable and close other software using the FM1 MIDI port.
3. Connect, check the **detected device ID**, leave the header choice on AUTO if it matches your known firmware family, and review the warnings.
4. The current firmware must approve **CHECK** before any transfer. The installer then identifies the actual OTA loader before **WRITE**.
5. Only a restart reporting **`FM-1_1500`** constitutes installer-verified completion. Save the diagnostic log if it stops at CHECK or LOADER.

The installer includes the B1 HF2-style handoff for known `ota-FM-1_9xx` and `ota-FM-1_015` loader families. **V1.50 native OTA** validates the destination FM1 app layout and chip key and stages ETCH’s pinned app-only loader instead of executing an unknown loader supplied by another custom-firmware package. The packaged boot header is not compared for app-only updates, because it is not overwritten. The official V15 full-flash restore remains exact-image pinned. A loader's MIDI port name alone is **not** proof of compatibility. No WebInstaller can force an unrelated running custom firmware or third-party updater to approve ETCH; never bypass an unknown loader's checks. Genuine stock V15 restoration requires the original verified vendor `.fwsc`, which is **not** bundled.

> **Do not interpret an offline-valid `.fwsc` as a brick-proof firmware.** Physical installation, sustained FM1 DSP load, flash persistence and independent recovery have not been demonstrated for V1.50 in this repository's test environment.

### Switching from ETCH to other firmware

V1.50 addresses the **outgoing** app-only OTA restrictions reported on GitHub: it checks the target FM1 chip key and application filesystem layout, stages ETCH's known app-only OTA loader, and leaves the already-installed protected bootloader area untouched. **It does not run an incoming unknown OTA executable.** Only the exact original official V15 package is permitted to perform a full-flash restore.

To replay an original third-party package against the V1.50 native updater **without connecting hardware**, run:

```bash
python tools/replay_external_fm1_ota.py --package /path/to/felucca-1.6.fwsc --version felucca-1.6
```

For 1.1.5 use `--version unverified`, and independently verify the original file's provenance and checksum. **Neither original Felucca 1.6 nor 1.1.5 has been replayed in this release environment, and physical cross-firmware flashing has not been confirmed.** The staged loader's real identity is `ota-FM-1_015`; installers that insist on a different loader identity may still require the verified stock V15 bridge.

If you are **still running V1.38**, its old CHECK restrictions remain active until it is upgraded; installing the new WebInstaller alone cannot change V1.38's OTA policy. Where your currently installed ETCH release accepts it, use the V1.50 **V15-header** update first, or follow the official verified V15 restore route. Do not repeatedly flash if your updater reports an unrecognized loader.

## Source and development

| Location | Contents |
|---|---|
| [`firmware/fm1-native/`](firmware/fm1-native/) | Device-native firmware, DSP, LCD, input, USB, OTA and project storage |
| [`engine/`](engine/) | Host-side engine and supporting logic |
| [`sim/`](sim/) | Browser simulator (`sim/index.html`) |
| [`WebInstaller.html`](WebInstaller.html) | Current USB MIDI OTA WebInstaller |
| [`release/`](release/) | Firmware images and current release manifests |
| [`tools/`](tools/) | Firmware packaging, build and validation tools |
| [`tests/`](tests/) | DSP, UI, project-format and OTA regression tests |
| [`.github/workflows/test.yml`](.github/workflows/test.yml) | Automated test workflow |
| [`CHANGELOG.md`](CHANGELOG.md) | Development and release history |

**Building** requires Python 3 and an independently obtained JieLi **pi32v2** toolchain. The vendor's original V15 `.fwsc` is a separate input where required; it is not bundled.

```bash
export JIELI_TOOLCHAIN=/path/to/jieli-toolchain
python3 firmware/fm1-native/build.py --storage --updater
npm test
npm run test:ota1484
npm run test:v149
```

See [`RELEASE-MANIFEST.json`](RELEASE-MANIFEST.json), `tools/build_v149_release.py` and the relevant build scripts in `tools/` for reproducible package generation. `npm test` validates source behaviour and installer data; it does **not** perform a physical flash.

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

[Source](https://github.com/SikkenWorks/ETCH) · [Documentation](README.md) · [Changelog](CHANGELOG.md) · [License](LICENSE)

</div>
