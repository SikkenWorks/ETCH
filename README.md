## ETCH V1.84 — WAVE AMP

See [README-V1.84.md](README-V1.84.md) for the audio-rate waveform-to-volume feature, PJT32 compatibility and implementation/test notes.

> **V1.83 UI ALIGNMENT (software-tested release candidate).** All 63 existing screen routes remain intact. Shared 4-parameter footers use centered bounded labels and values. MORPH uses centered, non-overlapping label/value pairs, including extreme values. All versioned UI headers display `V183 S` or `V183 A` right-aligned on the same line, refreshing immediately when SIMPLE/ADVANCED changes. Original firmware identity `FM-1_1500`, protected boot header, OTA loader, `PJT31` project format, PHOSPHOR palette, FX BYPASS and live preset manager are preserved. See `docs/V1.83-UI-ALIGNMENT-AUDIT.md`. No physical FM1 validation.

> V1.82: PHOSPHOR UI palette and EDIT long-hold flat-line confirmation. Previous project formats and package identity are retained.

> **V1.82 Advanced FX Bypass (software-tested candidate).** In ADVANCED workflow, hold the physical **FX** button for at least **700 ms** to open the **FX BYPASS** list (the normal short FX press still navigates the existing pages). The page lists up to 22 configured effects: 17 global effects, four Wave A/B inserts and Wave C FX. **K1** chooses an entry, **K4 or SEL** toggles **ON/BYP**, **K2** bypasses all listed effects, **K3** restores all listed effects, **FX** returns. Runtime mask only: original sound controls, PJT31 storage, `.etchpreset` and project values are not modified. Bypass skips the heavy chorus/delay/reverb and applicable inserts; some minimal buffer/state housekeeping continues, so zero CPU cycles is not claimed. No physical FM1 performance results yet. Previous 62 pages plus the new page yield **63 total page IDs**; the original pages and their navigation are retained. Identity remains **`FM-1_1500`**. See `docs/V1.82-FX-BYPASS-AUDIT.md` and `docs/ETCH-V1.82-COLOR-REFERENCE.docx`.

> **V1.80 Brand cleanup (software-tested candidate).** Removed only the decorative horizontal rule beneath **BORN OUT OF PASSION.** from the clean SIKKENWORKS boot hold frame. RGB/VHS transition, slogan, 1-second hold, workflow prompt, safe storage confirmation and all 62 pages remain. Normal ETCH package identity **`FM-1_1500`**, project format **`PJT31`**, boot region and trusted OTA loader remain unchanged. Hardware behavior has not been verified.

> **V1.79 VHS Boot + No-Flicker Storage UI (software-tested candidate).** On a normal cold boot, ETCH shows nine short **RGB/VHS-style glitch frames** (about 42 ms requested per frame, plus actual LCD transfer time), ends on a completely stable **SIKKENWORKS / BORN OUT OF PASSION.** screen for **1 second**, then opens the **ETCH SIMPLE / ADVANCED** selection. Turn **K1** to select SIMPLE (default) or ADVANCED, press the physical **SEL button** to confirm. The selection affects only the UI workflow in RAM; no screens or project settings are deleted. If foreign storage exists, the existing protected **FORMAT FOR ETCH? YES/NO** prompt follows the mode selection. That prompt is now rendered into an off-screen RGB565 buffer; it no longer clears visible LCD RAM on every input/hold tick. Changes are sent only when the content changes, including quantized SAVE-hold progress. **NO remains the default**; the existing explicit YES/SEL/3-second SAVE format safeguard is unchanged. SAFE/REC, OCT- ROM recovery, and SEQ keyboard-boot retain precedence and bypass the intro question. **No on-device LCD, audio or flash testing has been performed.** Hardware recovery is still mandatory before flash. Firmware identity **`FM-1_1500`**, project format **`PJT31`**, and the original loader/bootheader are unchanged. See `docs/V1.79-BOOT-UI-AUDIT.md`.

> **V1.78 Safe First-Boot Storage Setup (software-tested release candidate).** When the **ETCH-owned target project banks** are occupied by foreign or damaged data (`STORAGE LOCKED`), the FM1 displays **FORMAT FOR ETCH? YES/NO** at boot. **NO is the default** and never writes flash. YES requires K1 selection, a physical SEL confirmation, releasing SAVE and then holding SAVE for three seconds. This permanently deletes the data in **Felucca sample slots 2 and 3 (`0xB4000–0xDBFFF`) only**, and creates an ETCH project with read-back verification. The loader, OTA staging, recovery, legacy ETCH banks, sample slot 1, and user-preset sectors are not erased. Valid ETCH projects cannot be formatted through this startup route. Read errors and Safe Mode do not offer the wipe; NO keeps the old data, with ETCH storage still locked. This is **not** a full-device format. A physical FM1 test and independent recovery remain outstanding. **Back up other-firmware samples before choosing YES.** V1.77 `.etchpreset` live export/import and SAVE feedback remain unchanged. See `docs/V1.78-BOOT-STORAGE-AUDIT.md`.

> **V1.77 RAM Preset Rescue & SAVE Feedback (software-tested candidate).** The new standalone `ETCH-Project-Preset-Manager.html` uses read-only USB-MIDI snapshot requests to capture the live sound, including unsaved waveforms, and saves `.etchpreset` files with SHA-256/CRC32. Upload restores the sound to RAM only after bounded, CRC-checked transfer. No storage ownership, partition, project serialization (`PJT31`), OTA handshake, firmware package identity (`FM-1_1500`), or recovery mechanism is modified. Save screens show SAVING / VERIFYING before the existing verified-success / failure outcome. **V1.76 and earlier do not support live snapshots:** upgrading resets RAM and will not rescue an unsaved sound. **Do not power off, wipe, flash or reboot the unit that holds your unsaved patch.** The source does not automatically unlock foreign storage. Third-party or damaged flash ownership requires independent inspection and recovery planning. Hardware transfer and actual flash safety remain unverified.

> **V1.76 ORGANIC Drift Smoothing (software-tested candidate).** Fixed the audible jumps in ORGANIC DRIFT: the old random targets were chased in ~12ms and then held, producing distinct tuning steps. Drift now interpolates with zero-speed endpoints between bounded random targets every **2.5–9 seconds**, according to RATE. Turning DRIFT to 0 on a held note fades the moving offset smoothly back to neutral in ~1.5 seconds. VARIANCE remains per-note static; PHASE retains its previous behavior. The same controls/settings (`PJT31`) are used; there is no project migration or new persistent RAM. All 62 screens, STRING ENS, undo/redo RAM savings, USB, audio, project/OTA features and product identity **`FM-1_1500`** remain. Hardware/aural FM1 tests and flash safety are still outstanding. See `docs/V1.76-ORGANIC-DRIFT-REPORT.md`.

> **V1.75 RAM recovery (software-tested).** The native wave editor now shares one exact, reversible 16-snapshot ring between undo and redo. A full 16-step undo and 16-step redo after undo are retained, as are all 62 page IDs, STRING ENS, audio, FX, USB, MIDI, storage, projects and OTA. JieLi 48 kHz UAC static `.pool` usage decreases **344,060 → 331,772 bytes**, leaving **12,292 bytes free** instead of 4; no reserved SRAM region was moved. 25,032 randomized reference-model undo/redo operations, native regressions and native OTA packet replay passed. This does **not** establish runtime stack headroom, physical FM1 stability or safe flashing. Preserve existing recovery precautions. Product identity: `FM-1_1500`; project format: `PJT31`. See `docs/V1.75-RAM-OPTIMIZATION-REPORT.md`.

> **V1.74 DSP Performance & SRAM Audit (software-tested candidate).** Four Q15 one-pole updates (low/high EQ, high-pass and SPACE damping) now use provably bounded signed 32-bit products instead of 64-bit multiplies. Both calculations remain integer divisions with identical truncation. Four deterministic PCM reference scenes match V1.73 byte-for-byte, and targeted host regressions including synth/FX, USB audio, projects and patch switching passed. New `tools/v174_memory_audit.py` rejects linker-memory regressions and stack guard drift; `.pool` remains **344,060 / 344,064 bytes**, with only 4 bytes free. No claim of RAM reclaimed or measured on-device CPU speedup. All 62 screen IDs, STRING ENS, project format **PJT31**, USB/OTA code, GPL credits, loader and package identity **FM-1_1500** are preserved. The primary release package is `ETCH-V1.74.fwsc`; real FM1 and recovery verification are still outstanding.

> **V1.73 LCD efficiency & bounded compositor (software-tested candidate).** ETCH now draws standard 5×7 glyphs directly into the existing RGB565 canvas where possible, avoiding thousands of redundant per-pixel fill dispatches during multi-page UI redraws. A conservative fallback retains the original path for partial/out-of-canvas glyphs and large boot lettering. The compositor now refuses invalid canvas rectangles before touching RAM or submitting DMA. The 62/62 ADVANCED screen routes and STRING ENS factory/preview sound from V1.72 are unchanged. All 8,192 random RG565 glyph equivalence tests and static 240×240 literal-label bounds passed. The DSP pool is still effectively full: this release **does not reclaim RAM**, and pixel QA on a real FM1 remains unperformed. Firmware identity **FM-1_1500** and project format **PJT31** are unchanged.

> **V1.72 Screen Catalog + String Ensemble (software-tested candidate).** SIMPLE remains a five-screen sound-design flow; no screen ID is removed or renumbered. In **SIMPLE**, press **GLO** to reach WORKFLOW; turn K2 to **ADVANCED**, K3 to select one of the original **62 pages**, and K4 to open it. MONO-only pages tell you when MONO is required rather than silently changing the voice mode. Special NAME, PROJECT SAVE and PRESETS pages use their existing initializers. Returning via HOME is unchanged. K1 twice on WORKFLOW previews the **STRING ENS** vintage string-ensemble-inspired sound even on existing projects (wait for notes and voices to release). This only changes the working sound in RAM. It does **not** overwrite a stored preset or user waveform; use explicit SAVE to keep it. A separate STRING ENS factory preset is available as slot 17 in newly initialized factory banks without removing the original 16 starters. No authentic Solina divide-down or triple-BBD chorus circuit is claimed. Native project format **PJT31**, boot/OTA assets, product identity **FM-1_1500** and full USB/audio engine are preserved. Target hardware testing is still required.

> **V1.71 Workflow / UI Safety (software-tested candidate).** New **SIMPLE** default and **ADVANCED** workflow. SIMPLE routes SELECT through SOUND → MORPH → FILTER → FX → LIBRARY, keeping the original 62 editors in ADVANCED. GLO opens SYSTEM / WORKFLOW in SIMPLE; turn **K2** to choose the view, press GLO to return. Advanced routing remains intact. The chosen view is **runtime only** and resets to SIMPLE after reboot. Fixed a V1.70 fourth-knob-label off-by-one bug, bounded numeric values to their respective columns and protected auxiliary page titles from warning overlays. The source and firmware preserve **FM-1_1500**, **PJT31**, boot/OTA infrastructure, and existing audio/USB functionality. This is not physical-FM1-qualified; the tight DSP pool is unchanged.

> **V1.70 Sound-first UX + USB audio stability (software-tested candidate).** PLAY now shows a simplified SOUND screen with MORPH / ETCH / TONE / SPACE. FX first opens the four-main-effect overview before the existing advanced editors. Sixteen named sound starters seed **newly initialized** banks only; existing user patches/projects are not replaced. All native pages and audio/MIDI/OTA features remain available. LCD glyphs in the main parameter columns and titles are clipped to nonoverlapping regions; an interrupt-pressure-aware UI scheduler yields LCD time to audio/USB. The official `ETCH-V1.70.fwsc` retains `FM-1_1500` and `PJT31`. Hardware flashing and physical UI/auditory testing remain unverified.

> **Preserved V1.61 USB Audio RAM Fix:** The full UAC-enabled firmware can now link with 44.1/48 kHz USB capture, with 4,004 bytes free in normal SRAM and 1,588 bytes free in dedicated 8 KiB USB scratch SRAM (48 kHz build). Bootloader/USB product identity remains `FM-1_1500`; project schema remains PJT31. This is software-verified only, not FM1 hardware-qualified. The current firmware release candidate is `ETCH-V1.83.fwsc`; any stock-boothead ETCH container in the WebInstaller is a legacy compatibility option, not an official stock restore.

<div align="center">

# ETCH

**DRAW · MORPH · MODULATE**

*Open-source synthesizer firmware for the M-VAVE FM1*

**SikkenWorks** · Born out of passion.

[**Getting started**](#getting-started) · [**Controls**](#controls-and-shortcuts) · [**Sound engine**](#how-etch-works) · [**Firmware installation**](#installation-and-recovery) · [**Credits**](#credits-and-licensing) · [**Changelog**](CHANGELOG.md)

</div>

---

## V1.83 Quick start and 63-screen catalog

1. Start in **SOUND**. SELECT cycles MORPH, FILTER, FX and LIBRARY in SIMPLE.
2. Press **GLO**. On the WORKFLOW tab, turn **K2** to ADVANCED.
3. Turn **K3** to browse **all 63 page IDs (62 original + FX BYPASS)**. Turn **K4** to open the selected page. In POLY, mono-only sound editors show a clear requirement instead of silently switching mode.
4. For strings, from the WORKFLOW tab turn **K1 twice** with no notes playing. The built-in **STRING ENS** audition loads a warm saw/organ waveform blend, long release, subdued modulation and animated stereo chorus. No user preset or project is automatically overwritten.
5. **SAVE** the resulting patch explicitly into your chosen slot to keep it; use HOME to return to SOUND.

> The string audition uses the existing ETCH audio engine and two-tap chorus. It aims for vintage ensemble character, **not a verified emulation** of the original Solina's divide-down generators and triple-chorus circuitry. Voice count remains the existing ETCH voice limit.

## V1.71 Quick start (FM1 hardware)

1. Start at **SOUND**: K1 MORPH, K2 ETCH (Wave C depth), K3 TONE (cutoff), K4 SPACE (reverb).
2. Turn **SELECT** to visit MORPH, FILTER, the basic FX overview and WAVE LIBRARY, then back to SOUND. Long-press HOME opens preset browsing.
3. Press **GLO** to open SYSTEM → WORKFLOW. Turn **K2** toward ADVANCED for the complete original FX, MOD, ARP, SEQ and editor page routes. Press GLO or HOME to return to SOUND.
4. Return to SIMPLE with GLO → SYSTEM, select WORKFLOW with SELECT as needed, then turn K2 back.

SIMPLE **does not edit or overwrite a patch** and is not saved to projects. No menus or DSP controls are deleted. The button layout follows the firmware's native panel mappings, subject to physical hardware verification.

## What is ETCH?

ETCH turns the **M-VAVE FM1** into a synthesizer built around waveforms you create yourself. Instead of selecting a fixed FM algorithm, you draw a waveform, shape a second waveform, morph between them, and use a third waveform as a modulation source.

The entire instrument is designed for hands-on sound design, using the FM1's own screen, encoders and keyboard. It also supports external USB MIDI, saved sounds, pattern creation, effects, keyboard splits and live performance controls.

**Current release:** V1.78 · **Fixed package/USB identity `FM-1_1500`** · Project format `PJT31` · GNU GPL-3.0-only.

> **Hardware status:** V1.73 is a **software-tested firmware candidate**, not a hardware-qualified OTA release. Real Felucca 1.6/1.1.5 `.fwsc` packages and an actual FM1 were not available for the V1.70 tests. The updater has been exercised with real ETCH release packages and synthetic compatible-loader variants only. Compiling, hashing or simulating a transition cannot prove device boot, flash preservation or recovery. Back up your data and use only a separately proven recovery method before flashing.

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

### Make a usable sound quickly (V1.70)

1. Turn MASTER down to a comfortable level. Play a key: the SOUND screen is the default.
2. Turn **K1 MORPH** to blend two wave shapes. Turn **K2 ETCH** for character/modulation.
3. Use **K3 TONE** to open/close the filter; use **K4 SPACE** for ambience.
4. **Hold HOME** to browse saved presets. For new/erased factory banks, choose one of the 16 named starter sounds such as WARM KEYS, GLASS PAD, or DUST BASS. This does not replace existing user presets in an already initialized bank.
5. Press **FX** once to see DRIVE / CHORUS / DELAY / REVERB together. Press FX again for advanced editors. Use **SELECT** on FX editors to browse; all effects remain present.
6. To design a new waveform, press **EDIT**. To see A/B wave sources, turn **SELECT** from SOUND to MORPH, FILTER or LIBRARY. Keep complex editors optional until needed.
7. **SAVE** deliberately when you like a result. Existing PJT31 presets/projects are unchanged by the UI upgrade.

**Tip:** For an immediate musical sound, begin with a low ETCH depth, a moderately open TONE, and a small amount of SPACE. Increase modulation and DRIVE only after the dry sound is useful.

1. Turn down the output volume before experimenting with new waves or DRIVE.
2. Press **EDIT** to enter DRAW. Use **SELECT** to choose Wave A, B or C.
3. Use **K1** to move along the waveform and **K2** to change its shape. Use **K3/K4** to select and adjust harmonics.
4. Press **HOME**, open **MORPH** with SELECT and blend A and B. Open MOD WAVE to bring in Wave C.
5. Adjust the envelope, filter and effects; play the FM1 keyboard or a USB MIDI controller.
6. **SAVE** the waveform, preset or project using the appropriate save context.

The physical display is the authority for context-dependent controls. The tables below describe the implemented V1.70 layout, not the older concept mockups.

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

**The complete WebInstaller is [`WebInstaller.html`](WebInstaller.html).** V1.70 has **one public release**: [`release/ETCH-V1.70.fwsc`](release/ETCH-V1.70.fwsc), which includes 44.1/48 kHz USB audio. The WebInstaller retains a hidden [legacy stock-boothead ETCH variant](compat/ETCH-V1.70-LEGACY-STOCK-HEAD.fwsc) only for previously identified older installations. Both install **ETCH**, never official stock firmware. The identity remains exactly **`FM-1_1500`**, and the OTA loader, 16 KiB bootheader and project format PJT31 remain protected. On-device USB audio and cross-firmware restore are not yet validated.

1. Back up projects, waveforms and calibration data. Keep your original firmware and a **tested**, independent recovery path available.
2. In Chrome or Edge, open the WebInstaller over HTTPS/localhost, close other software using the FM1 MIDI port, and connect the FM1 using a data-capable USB cable.
3. Confirm the detected running firmware and choose the compatible protected boothead. The running firmware must approve CHECK; an installer cannot bypass that.
4. The device must stage and read back its known OTA loader, receive the host's literal `success` response, and restart into the loader. The installer verifies the loader identity before sending the firmware.
5. When the loader reports WRITE completion and the FM1 reconnects, **confirm `V160` on the device display**. The SysEx firmware identity remains `FM-1_1500` across V1.50 and V1.60, so that identity alone cannot distinguish their software versions.

The WebInstaller recognizes approved `ota-FM-1_9xx` and `ota-FM-1_015` handoff identities. The latter is the identity of the loader that V1.60 actually stages. MIDI port names by themselves are not trusted. Official V15 restoration remains an **entirely separate**, strictly pinned stock-image path and requires a genuine stock `.fwsc` from an authorized source. The stock-header ETCH variant is **not** a stock restore.

**Important:** the `.fwsc` files are structurally verified candidates, **not proven safe to flash on your FM1**. Hardware recovery and the complete cross-firmware transition have not been demonstrated in this release environment.

### Switching from ETCH to compatible firmware

The outgoing OTA path in `firmware/fm1-native/ota.c` no longer has a fixed *incoming* Felucca loader size or a single-binary SHA allowlist for app-only packages. It validates the UFW directory, every package payload CRC, the incoming two-level OTA header, the versioned `FELUCCA-LOADER-1` marker in **decoded** LZ4 data, the complete decoded loader CRC, chip key `0x980F`, XIP `0x02000120`, application offset `0x4000`, the full decrypted app-area CRC and `app.bin` CRC. It rejects unsupported structures, unknown families, corruption and flash-layout conflicts **before staging flash is erased**.

The incoming loader binary is checked but **never executed**. The running ETCH app stages only its SHA-256-pinned, 6,898-byte, app-only writer, under the existing `ota-FM-1_015` identity. The original protected boot region, project storage and unrelated flash areas remain outside its intended writes. The incoming `flash.bin` boot bytes are not required to match the installed bootloader, because app-only updates do not program them. Stock full-flash restore remains limited to the independently pinned V15 image and stock loader.

To test an authentic third-party `.fwsc` with the actual native OTA code, without connecting a device:

```bash
python tools/replay_external_fm1_ota.py --package /path/to/felucca-1.6.fwsc --version felucca-1.6
python tests/test_v160_cross_loader.py
```

The first command needs an actual, independently verified Felucca package. The second creates a **synthetic** variant of the existing ETCH loader with a different compressed length, then checks acceptance, staging and rejection paths. It is **not** evidence that every external Felucca build works. See `docs/V1.60-OTA-VALIDATION.md` for measured software results and the still-required physical tests.

The old CHECK behaviour remains on devices running older ETCH versions until those devices are upgraded. A newly downloaded installer cannot rewrite the old running firmware's validation policy. Stop if the running firmware rejects a new package and use a previously verified update/recovery path, not repeated blind attempts.

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
