# ETCH — User Manual

**Draw. Morph. Modulate.**  
**SikkenWorks · Born out of passion.**

**Firmware:** ETCH V1.85 · **Instrument:** M-VAVE FM1  
**Project:** [github.com/SikkenWorks/ETCH](https://github.com/SikkenWorks/ETCH)  
**Firmware identity:** `FM-1_1500` · **Current project format:** `PJT32`

> [!IMPORTANT]
> **Experimental firmware.** V1.85 has passed software builds and host tests, but not a complete physical FM1 qualification. Firmware flashing and recovery are not guaranteed. Keep a verified backup and a proven independent recovery route before updating. **Never reboot, wipe or flash while an important sound exists only in RAM.**

## Contents

1. [Start here](#1-start-here)
2. [Controls and shortcuts](#2-controls-and-shortcuts)
3. [Make your first sound](#3-make-your-first-sound)
4. [Draw, edit and save waveforms](#4-draw-edit-and-save-waveforms)
5. [Morph and modulation](#5-morph-and-modulation)
6. [WAVE AMP: a waveform controls volume](#6-wave-amp-a-waveform-controls-volume)
7. [Envelopes, filters and effects](#7-envelopes-filters-and-effects)
8. [Advanced sound design](#8-advanced-sound-design)
9. [Arpeggiator, sequencer and harmony](#9-arpeggiator-sequencer-and-harmony)
10. [GROUPS, Performance and Motion](#10-groups-performance-and-motion)
11. [Presets, projects and storage](#11-presets-projects-and-storage)
12. [Project and Preset Manager](#12-project-and-preset-manager)
13. [USB, startup options and updating](#13-usb-startup-options-and-updating)
14. [Troubleshooting](#14-troubleshooting)
15. [Quick reference and screen directory](#15-quick-reference-and-screen-directory)
16. [Technical information and credits](#16-technical-information-and-credits)

---

## 1. Start here

ETCH is a synthesizer for the FM1 built around **three editable waveforms**:

- **Wave A** and **Wave B** form one sound. **MORPH** changes the shape between them.
- **Wave C** can modulate that sound's phase, morph position or selected effect parameters.
- **WAVE AMP** can also use Wave A, Wave B or their morph as a fast, repeating *volume contour*.

Shape the result with filtering, an envelope and effects. You can save complete sounds as presets or reusable waves in the waveform library.

### What happens at startup?

1. The RGB/VHS-style boot animation ends on **SIKKENWORKS / BORN OUT OF PASSION.** for one second.
2. ETCH asks **SIMPLE** or **ADVANCED**. Turn **K1**, then press **SEL** to confirm.
3. If incompatible storage is detected, ETCH may ask whether you want to format the designated ETCH storage area. See [Storage setup](#storage-locked-and-first-boot-formatting) before choosing YES.

| Workflow | Use it for | Navigation |
|---|---|---|
| **SIMPLE** | Fast sound design | SOUND → MORPH → FILTER → FX → LIBRARY |
| **ADVANCED** | All sound editors and functions | Full menus and the 64-screen catalog |

**Neither mode removes features or changes the sound engine.** You can switch later: press **GLO** in SIMPLE to open **SYSTEM / WORKFLOW**, then turn **K2** to select ADVANCED. On WORKFLOW, **K3** chooses a screen and **K4** opens it when allowed. MONO-only pages cannot be opened in POLY. In ADVANCED, use **GLO** to reach SYSTEM, then rotate **SELECT** to the WORKFLOW tab. The workflow choice is not saved across power cycles. The version label shows **S** or **A** next to the version.

### Three kinds of saved data

| Type | Contains | Capacity |
|---|---|---|
| **Waveform** | A reusable, named 128-point single-cycle wave | 64 library slots |
| **Preset** | A named sound, including waves and synth/FX settings | 32 preset slots |
| **Project** | Working state, performance setups, sequences and related data | 4 project slots |

A **`.etchpreset`** is a separate computer backup of an active sound. It is **not** a complete project export.

---

## 2. Controls and shortcuts

The screen always shows the function of **K1–K4** for the current page. **SEL** is a button; **SELECT** is a rotary encoder. They are different controls.

| Control | Main action |
|---|---|
| **MASTER** | Physical output volume |
| **K1–K4** | Edit the four on-screen parameters |
| **SELECT encoder** | Browse pages, select waves, steps or editor tabs |
| **ALGORITHM encoder** | Contextual editing; DRAW DETAIL/SOFT/EDGE brushes; scenes or sequencer inspectors |
| **PRESETS encoder** | Change BPM in normal use; browse sounds inside PRESETS |
| **HOME** | Return to SOUND / exit an editor |
| **EDIT** | Enter or leave DRAW; long hold asks before resetting current wave |
| **ENV** | Open envelope; Undo during DRAW |
| **LFO** | Cycle modulation editors; Redo during DRAW |
| **FX** | Effects pages; waveform FX during DRAW |
| **ARP / SEQ** | Arpeggiator / sequencer pages |
| **GLO** | Harmony, SYSTEM and WORKFLOW access |
| **SAVE** | Save or confirm a context-dependent operation |
| **REC / PLAY/STOP** | Context-dependent recording and transport |
| **OCT− / OCT+** | Shift keyboard octave |

### Essential shortcuts

| Action | How |
|---|---|
| Go to SOUND | Tap **HOME** |
| Browse named presets | Hold **HOME** about 1.5 seconds, then turn **PRESETS** |
| Open PROJECTS | Press **HOME + SAVE** together |
| Open GROUPS | Press **HOME + FX** together |
| Open PERFORMANCE | Press **HOME + ARP** together |
| Switch MONO / POLY | Press **OCT− + OCT+** together |
| Open FX BYPASS | In ADVANCED, hold **FX** about 700 ms |
| Clear the editable wave | In DRAW, hold **EDIT** about 700 ms; confirm YES/NO |
| Change workflow | **GLO → SYSTEM / WORKFLOW → K2** |
| Open any allowed screen | **WORKFLOW → K3 SCREEN → K4 OPEN** in ADVANCED |
| Restart after AUDIO STOP | Hold **HOME + PLAY/STOP** about 1 second, if responsive |

**Do not use the old double-HOME shortcut for projects.** The current shortcut is **HOME + SAVE**.

---

## 3. Make your first sound

1. Turn **MASTER** down, play a key and raise the volume carefully.
2. Open **SOUND** with **HOME**. Try the four immediate controls:

   | Knob | Sound control |
   |---|---|
   | **K1 MORPH** | Blend the shape of Wave A and B |
   | **K2 ETCH** | Wave C modulation depth |
   | **K3 TONE** | Filter cutoff / brightness |
   | **K4 SPACE** | Reverb / spatial amount |

3. Turn **SELECT** to **MORPH**. Use **K1/K2** to select Wave A/B and **K3** to blend them. **K4** adjusts the Wave C rate/ratio; **ALGORITHM** adjusts ETCH depth.
4. Open **FILTER** and use K2 cutoff and K3 resonance to refine the sound.
5. Open **FX** and add a little chorus, delay or reverb.
6. **SAVE** your result as a named preset. Wait for confirmation before turning off the FM1.

**Want a fast starting point?** Hold HOME and choose a factory sound such as **WARM KEYS**, **GLASS PAD** or **DUST BASS** (when available in your bank). **STRING ENS** is a vintage string-ensemble-inspired preset in newly initialized banks. On the WORKFLOW page you can also turn **K1 twice** to audition STRING ENS in RAM, without overwriting a stored sound; release held notes first.

---

## 4. Draw, edit and save waveforms

Press **EDIT** to enter DRAW. Turn the **SELECT encoder** to choose **Wave A, B or C**.

| Control | DRAW function |
|---|---|
| **K1** | X position: select any of the 128 waveform points |
| **K2** | Y position: raise/lower the selected point with the current brush |
| **ALGORITHM encoder** | Switch DETAIL / SOFT / EDGE drawing brush |
| **K3** | Choose harmonic / PART |
| **K4** | Add or remove harmonic gain |
| **ENV** | Undo |
| **LFO** | Redo |
| **FX** | Waveform effects and SCULPT |
| **SAVE** | Save the wave to the library |
| **HOME / short EDIT** | Leave DRAW |

DRAW previews the selected waveform on its own, without the complete morphed sound. The original patch settings return when you leave DRAW.

### Brush types and smoother drawing

| Brush | Shape | Best for |
|---|---|---|
| **DETAIL** | Small five-point shaped brush | Fine tuning and localized harmonics |
| **SOFT** | Broad 17-point smooth curve | Rounded waves, warm pads and gradual contours |
| **EDGE** | One directly edited point | Sharp edges, overtone-rich waves and unusual timbres |

Rotate the **ALGORITHM encoder** in DRAW to change brushes. All **128 waveform points** are available, rather than a reduced number of drawing positions. Horizontal and vertical editor input is checked every **25 ms**; audible waveform recompilation is throttled to roughly one update per **70 ms** during sustained edits to protect the sound engine. The on-screen cursor can therefore respond before you hear the newest edit. Pending edits are applied on leaving DRAW.

**Why different waves can sound similar:** a broad brush rounds off distinct shapes, a closed filter masks upper harmonics, and high notes use band-limited wavetable levels. Try EDGE, an open filter, and middle/lower notes to compare shapes; then try SOFT for deliberately smoother timbres. ETCH recompiles only modified wave banks, while leaving the existing band-limited tables intact.

### Start with a flat line

1. While in **DRAW**, hold **EDIT** for about 700 ms.
2. At **CLEAR CURRENT WAVE?**, turn **K1** to select **YES** or **NO**. NO is the default.
3. Press **SEL**. YES replaces **only the selected waveform** with a flat zero line; NO and HOME cancel.
4. Use **ENV (Undo)** to reverse the change if needed.

This clears the working wave in RAM, **not** an entire project or the flash storage. A short EDIT tap retains its ordinary action.

### Tools, WFX and SCULPT

In DRAW, use **FX** to reach **WFX 1 → WFX 2 → SCULPT** and the tool workflow.

- **TOOLS:** line, smooth, mirror, invert, normalize, flatten, random, sine, triangle, saw, square, phase rotate and Wave Genetics.
- **WFX 1/2:** waveform-level drive, bit reduction, chorus, delay, reverb, fold, smooth and mix. A confirmed **BAKE** applies the processing to editable wave points.
- **SCULPT:** fold, bend, asymmetry, phase and harmonic shaping, with preview before applying.

These edits change the **waveform shape**; they are not the same as the real-time FX chain. Undo/Redo keeps a shared history of up to 16 snapshots.

### Save or load a wave

**Save:** DRAW → SAVE → **K1** destination slot → **K2** name position → **K3** character → **K4** backspace → **SAVE** to commit/confirm.

**Load:** LIBRARY → **K1** slot → **K2** destination A/B/C → **REC twice** to confirm. Empty library slots may be skipped when browsing.

Saving a wave is separate from saving a preset that uses it. Save important changes in **both places** when you want a reusable wave and a finished sound.

---

## 5. Morph and modulation

### The basic signal flow

```text
Wave A ── A inserts ──┐
                      ├─ MORPH → Filter / Drive → WAVE AMP → ADSR / Voice gain
Wave B ── B inserts ──┘    ↑                               │
                           │                               └→ Global FX → Output
Wave C ──────────── phase / morph modulation
```

MORPH changes the **wave shape** at each oscillator position; it is not simply crossfading two independent audio channels.

### Main modulation controls

| Screen | K1 | K2 | K3 | K4 |
|---|---|---|---|---|
| **MORPH** | Wave A | Wave B | Morph | C rate |
| **MOD WAVE** | Wave C | Depth | Rate / ratio | PHASE / MORPH destination |
| **WAVE C FX** | FX destination | Depth | Rate | Wave C |

On **MOD WAVE**, the **SELECT encoder** changes KEYED, FREE or TEMPO rate mode. Try low depth first; audio-rate modulation can radically change the timbre.

### Mod Matrix

Four independent routes let one source change another parameter in real time:

| Knob | Setting |
|---|---|
| **K1** | Route 1–4 |
| **K2** | OFF, VELOCITY, ENVELOPE, WAVE C, LIVING, AFTERTOUCH or KEYTRACK |
| **K3** | MORPH, C DEPTH, FILTER or SPACE |
| **K4** | Signed modulation amount, −100 to +100 |

**AFTERTOUCH** needs a MIDI controller that sends channel pressure. Use SOURCE OFF or AMOUNT 0 to disable a route. Matrix modulation does not permanently redraw your waveforms.

---

## 6. WAVE AMP: a waveform controls volume

**WAVE AMP** turns an existing drawn wave into a **repeating, audio-rate volume shape**. Its timing is tied to each note's oscillator phase. It is not a slow envelope that runs once per keypress.

**Open:** ADVANCED → press **LFO** until **WAVE AMP**, or open it from the WORKFLOW screen catalog.

| Control | Function |
|---|---|
| **K1 SOURCE** | 0 OFF · 1 Wave A · 2 Wave B · 3 MORPH |
| **K2 DEPTH** | 0–100% |
| **K3 / K4** | Unused on this screen |

At **100% DEPTH**, the highest waveform level produces full volume and the lowest level produces silence. At lower depths, some of the unmodulated volume remains. The volume contour follows the oscillator cycle, so it can create new harmonics and textures, especially at higher notes.

**Try this:** draw a rounded shape in Wave A, play a sustained note, set WAVE AMP SOURCE to A and increase DEPTH slowly. Compare A, B and MORPH. To return to the original sound, select **OFF** or set DEPTH to **0**.

WAVE AMP uses the existing A/B oscillator values. DEPTH is shown as 0–100% but stored in 64 discrete levels. It does **not** require another waveform, oscillator or separate modulation buffer. The standard ADSR still works. Its SOURCE and DEPTH are saved in **PJT32** and supported by the V1.85 Preset Manager.

---

## 7. Envelopes, filters and effects

### Envelope and filter

| Page | K1 | K2 | K3 | K4 |
|---|---|---|---|---|
| **ENV** | Attack (ms) | Decay (ms) | Sustain (%) | Release (ms) |
| **FILTER** | Filter type | Cutoff | Resonance | Drive |

Use a short attack for plucks and a longer attack for pads. ETCH uses a **256-sample smooth note-on fade** to reduce abrupt key transients, with rounded crossfades for wavetable changes and stolen voices. These transitions are separate from editable ADSR and do not require additional audio buffers. **Clicks and analogue startup/output pops are not guaranteed eliminated; physical listening tests remain necessary.**

### Global effects

The SIMPLE **FX** screen gives quick access to **DRIVE, CHORUS, DELAY and REVERB**. ADVANCED exposes these individual editors and the additional effects below.

| Effect pages | What you can adjust |
|---|---|
| DRIVE / CHORUS / DELAY / REVERB | Tone/drive, chorus movement, delay time/feedback and reverb space |
| REVERSE DELAY / RING MOD | Reverse delay; ring rate, shape and bias |
| CHANNEL EQ / COMPRESSOR | Tone balance, high-pass, dynamics |
| TREMOLO / VIBRATO | Level or pitch movement |
| PHASER / FLANGER | Sweeping modulation effects |
| DECIMATOR / WAVEFOLDER | Digital reduction and nonlinear timbres |
| TAPE INSTABILITY / ROTARY / AUTO-WAH | Tape motion, rotating speaker and envelope-sensitive filtering |
| WAVE A FX / WAVE B FX | Two pre-morph insert slots per wave: OFF, RING, BIT, FOLD, TREM or LP |

The detailed FX editors show their precise K1–K4 settings on the FM1 display. **WAVE A/B FX** use **K1 SLOT, K2 TYPE, K3 DEPTH, K4 MIX**. These inserts act before MORPH and do not overwrite the saved wave points.

### FX BYPASS: skip effect processing

In **ADVANCED**, **hold FX for about 700 ms** to open **FX BYPASS**. Only configured effects appear in the list, including global FX, Wave A/B inserts and Wave C FX.

| Control | Action |
|---|---|
| **K1** | Scroll through the effects |
| **K4 or SEL** | Toggle the selected effect between **ON** and **BYP** |
| **K2** | Bypass all listed effects |
| **K3** | Restore all listed effects |
| **FX (short)** | Go back |

**BYP** skips the expensive DSP processing where implemented, rather than simply turning the wet mix to zero. Some light state/housekeeping still runs. Bypass is **temporary in RAM**: the stored FX values are preserved and can be restored with their original settings. CPU savings have not yet been measured on a physical FM1.

---

## 8. Advanced sound design

The following editors remain available in ADVANCED. Some require **MONO** to avoid excessive multi-voice DSP load.

| Editor | Main purpose | Controls |
|---|---|---|
| **EVOLUTION** | Animate the waveform non-destructively | Depth, rate, shape (FOLD/SHARP/STAIR), sync |
| **ORGANIC** | Slowly evolving analogue-style pitch drift | Drift, rate, phase, variance |
| **LIVING** | Real-time changing harmonics | Depth, rate, color, variation |
| **SPECTRAL** | Edit selected harmonic content in a wave | Wave, partial, gain edit, tilt edit |
| **PATCH BLEND** | Interpolate two saved presets into the current sound | Preset A, preset B, mix, active |

**ORGANIC drift** now moves smoothly between targets, with a rate spanning roughly **2.5–9 seconds**, rather than snapping to them. Turning drift off on a held note also returns towards neutral smoothly.

**SPECTRAL** changes actual waveform data and can be undone. **PATCH BLEND** may show **WAIT NOTE OFF** while an expensive waveform rebuild is pending. Release notes and stop the sequencer before continuing. PATCH BLEND edits the current sound; it does not run two separate synth engines.

**MONO/POLY:** press **OCT− + OCT+** together. In POLY, the firmware hides or bypasses the heavier MONO-only modulation editors. **MOD WAVE, WAVE C FX, MOD MATRIX and WAVE AMP** remain part of the POLY LFO cycle. All settings are retained when switching modes. The voice budget is shared; effects and heavy patches can reduce safe polyphony.

---

## 9. Arpeggiator, sequencer and harmony

### ARP and RIFF

Press **ARP** to cycle **ARP → ARP EDIT → CLOCK → RIFF LAB** in ADVANCED.

| ARP control | Function |
|---|---|
| **K1** | Mode: OFF, UP, DOWN, UP/DOWN, PLAYED, RANDOM, CUSTOM, RIFF |
| **K2** | Rate |
| **K3** | Octave range |
| **K4** | BPM, or **NEW RIFF** in mode 7 |

**Make a riff:** set your key/scale, choose **mode 7 (RIFF)**, hold a note and turn **K4** to generate a new 16-step phrase. In **RIFF LAB**, use K1 density, K2 mutation, K3 variant and K4 new riff. Save your preset to retain it.

### Sequencer

The sequencer supports up to **256 steps**, up to **eight note triggers per step**, HOLD relationships and six parameter-lock destinations.

| Screen | K1 | K2 | K3 | K4 |
|---|---|---|---|---|
| **SEQ** | Length | Rate | Swing | Gate |
| **NOTES** | Note/voice | Velocity | Note length | Pitch |
| **LOCKS** | Lock destination | Value | On/off | Step |
| **CLOCK** | External follow | Gate | Swing | BPM |
| **WAVE SEQ** | Step | A / MIX / B | Morph % | Lock on/off |

**PLAY/STOP** runs transport. In the main sequence edit view, **REC** enables live recording. With transport stopped, choose a step using **SELECT** and use the keyboard to place or remove note triggers. In sequencer context, rotate **ALGORITHM** for advanced **GRID, HOLD, LOCKS, BAKE** and **CLOCK** screens.

The six lock targets are **MORPH, ETCH depth, FILTER cutoff, CHORUS, DELAY and REVERB**. **WAVE SEQ** chooses A, A/B mix or B from the *current patch* per step; it does not load arbitrary library slots independently on every step.

**Destructive actions:** GRID clear and harmony BAKE use confirmation steps. Do not confirm a destructive operation without checking the displayed step.

### Keys, scales and chord guidance

**HARM:** K1 root, K2 scale, K3 keylock (OFF/GUIDE/LOCK), K4 chord type. Nine modes are available: **Major, Dorian, Phrygian, Lydian, Mixolydian, Minor, Locrian, Harmonic Minor and Melodic Minor**.

**GLO** provides GUIDE / FIFTHS / SYSTEM in ADVANCED. GUIDE displays scale/chord information. Its **NOTE** control changes the displayed note/chord root for reference; it does not by itself transpose the synth or rewrite a pattern. **FIFTHS** helps navigate root notes by the circle of fifths.

---

## 10. GROUPS, Performance and Motion

### GROUPS: split the keyboard

Press **HOME + FX**. Two optional groups can play separate saved presets across different keyboard ranges, with MAIN playing outside those ranges.

1. **K1:** choose Group 1 or 2; **K2:** select a saved preset; **K3:** enable it.
2. **K4:** start range capture, then play the low and high key to set the bounds.
3. Ensure the ranges do **not overlap** and the end note is above the start note.
4. Save the **PROJECT** to keep the setup.

All groups share the synth's **eight-voice total budget** and the shared post-mix effects; each group does not receive eight additional voices.

### PERFORMANCE: four macros and two scenes

Press **HOME + ARP**. Four macros can control **MORPH, C DEPTH, CUTOFF, DRIVE or SPACE**.

- Press **SELECT** to switch between **LIVE** and **ASSIGN**.
- In ASSIGN, turn K1–K4 to choose targets. In LIVE, turn them to perform.
- Turn **ALGORITHM** to select scene **A** or **B**.
- Press **REC** to capture a scene and **PLAY/STOP** to recall it.
- Save a PROJECT to keep the assignments and scenes.

### MOTION: record knob movements

Go to **SYSTEM → MOTION** (SYSTEM's first tab, then REC). **REC** starts/stops motion recording; turn K1–K4 during the recording to capture Morph, ETCH depth, Cutoff and Space. **PLAY/STOP** starts/stops motion playback. Recording stores up to about **6.4 seconds** (64 frames at 100 ms each). Recording a new clip replaces the current working motion clip.

---

## 11. Presets, projects and storage

### Saving correctly

| Save what? | Procedure |
|---|---|
| **Waveform** | DRAW → SAVE → name/slot → SAVE/confirmation |
| **Preset** | Outside DRAW → SAVE → choose PRESET → name/slot → SAVE/confirmation |
| **Project** | **HOME + SAVE**, or SAVE outside DRAW → choose PROJECT → name/slot → SAVE/confirmation |

In the save editor, **K1** selects slot, **K2** selects a character position, **K3** changes the character, and **K4** backspaces. Use the **SELECT encoder** to choose PRESET or PROJECT in the save workflow when needed.

**Wait for the result.** ETCH can display **SAVING / VERIFYING**, then a verified success indication or a reason such as **STORAGE LOCKED**, **STORAGE FULL** or **VERIFY FAILED**. If the result says **NOT SAVED**, the sound is still in RAM only. Do not assume the save worked.

Hold **HOME** about 1.5 seconds to open the named PRESETS browser. Turn **PRESETS** to load sounds. The PRESETS encoder normally changes BPM outside that browser.

**Projects:** open with **HOME + SAVE**. Use **SEL** to open a selected project; **REC** opens Project Help. Project deletion is guarded by repeated **REC + SAVE** confirmation. It is permanent.

### STORAGE LOCKED and first-boot formatting

If ETCH finds foreign/incompatible data in its dedicated storage area, it may show **FORMAT FOR ETCH?** after the workflow selection.

- **NO (default):** keeps all foreign data, but the affected ETCH storage remains locked. No flash is erased.
- **YES:** selects formatting of the *designated ETCH-owned target region* **`0xB4000–0xDBFFF`**, which may contain older **Felucca sample slots 2 and 3**. Those samples will be **permanently deleted**.

**To confirm YES:** select with **K1**, press **SEL**, release SAVE, then **hold SAVE for 3 seconds**. HOME cancels. Do not interrupt while **FORMATTING / VERIFYING** is displayed. Valid ETCH projects or flash read errors should not be offered this destructive takeover route.

> [!WARNING]
> This is **not a whole-device format**. The bootloader, OTA staging, recovery and unrelated flash areas must not be erased by this operation. Nevertheless, the protection has only been verified in software. **Back up foreign-firmware samples before accepting YES.**

If you have an unsaved sound and storage is locked, use the [Project and Preset Manager](#12-project-and-preset-manager) to make a RAM snapshot before doing anything destructive.

### Project compatibility

ETCH writes **PJT32** projects, including WAVE AMP source/depth. It can read earlier **PJT31** projects, but older ETCH firmware may **not** be able to read PJT32 files. Back up before downgrading. Storage formatting does not replace the need for a backup.

---

## 12. Project and Preset Manager

The **ETCH Project & Preset Manager** is a standalone browser tool for reading the **currently active sound directly from FM1 RAM**. It can save that sound as `.etchpreset` and load a saved sound back into RAM.

### Back up an active sound

1. Connect the FM1 over a **USB data cable**.
2. Open **ETCH-Project-Preset-Manager.html** in desktop Chrome or Edge (Web MIDI/SysEx available in a secure context).
3. Click **Grant MIDI access**, choose the FM1 port and **Check ETCH protocol**.
4. Click **Read current sound**. Wait until the read/checksum completes.
5. Click **Save .etchpreset** and keep a copy on your computer.

### Restore a sound

1. Save or export the FM1's current sound first, because importing replaces the active RAM sound.
2. Click **Open .etchpreset** and choose the file.
3. Stop the sequencer and release held notes.
4. Click **Send to FM1 (RAM only)**.
5. If you want the sound to survive power-off, perform a separate **verified SAVE** on the FM1 after importing.

The format carries the **three waves and associated sound settings**, including WAVE AMP in the current version. It does **not** contain every project, sample or sequencer pattern. `.etchpreset` V1 and V2 are supported by the updated Manager. The 1,044-byte live snapshot is protected by CRC32 and SHA-256 in the exported file.

**Important:** live reading needs firmware **V1.77 or later** with the snapshot protocol. An old V1.76 sound cannot be recovered by installing V1.85, because firmware flashing clears RAM. This webapp never writes FM1 flash or issues OTA commands.

---

## 13. USB, startup options and updating

### USB MIDI and audio

ETCH handles USB-MIDI notes/controls and supported timing and firmware-update messages. USB-audio code can be built for 44.1/48 kHz variants, but long-term on-device behaviour and all performance combinations have **not** been hardware-qualified. When using Web MIDI, close any DAW or other program holding the FM1 MIDI port.

### Special boot keys

| Hold during power-on | Function |
|---|---|
| **SEQ** | USB **MIDI KEYBOARD MODE**; FM1 keys transmit MIDI, local synth audio disabled |
| **REC** | ETCH application **Safe Mode**; silent/read-only |
| **OCT−** | Request loader/UBOOT entry where supported |
| **GLO** | Diagnostic PC6 analogue-output mute route, for startup-pop investigation |

MIDI KEYBOARD MODE sends the FM1's 27 physical keys on MIDI channel 1. OCT− / OCT+ shift the transmitted octave. Restart without holding SEQ to return to normal synthesis. Safe Mode and a responding loader are **not** proof of independent ROM-level recovery.

### Updating ETCH

ETCH's public release uses a single regular `.fwsc` package with identity **`FM-1_1500`**. The WebInstaller communicates with the FM1 over USB-MIDI, validates update compatibility and uses a trusted loader path. A compatible loader handshake does **not** prove that switching to every other custom firmware or restoring stock is safe.

**Before flashing:** keep a full data backup, verify the release and checksum, and make sure an independent restore route has been tested on your actual FM1. Use desktop Chrome/Edge, a proper USB cable and the correct MIDI input/output. Never force an update after a compatibility rejection. The official stock restore route requires genuine, independently verified stock firmware and its supported loader.

> [!CAUTION]
> A successful browser transfer is not proof that the FM1 boots or that recovery will work after a failed flash. **V1.85 has software validation, not an on-device cross-firmware or full recovery qualification.**

---

## 14. Troubleshooting

| Symptom | What to do |
|---|---|
| **STORAGE LOCKED / cannot save** | Export your sound to `.etchpreset` first. Do not wipe or flash to rescue the sound. Review the protected storage setup and back up foreign data. |
| **No save confirmation** | Look for SAVING / VERIFYING and a final success or failure indication. Do not power off on uncertainty. |
| **A preset changed or vanished after reboot** | Confirm you saved a PRESET or PROJECT, not only a waveform. Verify storage was writable. |
| **WAVE AMP has little effect** | Use a visibly varied wave, SOURCE A/B/MORPH and increase DEPTH. Check that it is not OFF. |
| **WAVE AMP sounds harsh** | That can be normal at audio rate. Lower DEPTH or use a rounded SOFT brush. |
| **Drawing seems delayed** | The 25 ms input update and 70 ms wavetable compile limit are separate; audio may trail cursor motion briefly while drawing. |
| **Different waves sound alike** | Try EDGE at mid/lower notes, open FILTER cutoff, lower wet FX and compare waves alone in DRAW. High-note band-limiting intentionally removes upper harmonics. |
| **Key-on clicks / plops** | Test a clean wave at low output volume with low DRIVE/FX. Smooth note-on, wavetable and voice-steal fades reduce digital transients; analogue startup/output clicks may still occur. |
| **No sound from Wave C** | Check its rate, target and DEPTH; MOD WAVE at zero depth is effectively bypassed. |
| **An effect appears inactive** | Check FX BYPASS, wet MIX, insert TYPE and the effect's routing. |
| **ORGANIC / LIVING / SPECTRAL missing** | Check **MONO** mode. These are intentionally unavailable in POLY. |
| **PRESETS knob loads nothing** | Hold HOME to enter PRESETS. The same encoder controls BPM during normal use. |
| **ARP silent** | Hold a key, choose an active ARP mode and stop the sequencer if it has priority. |
| **Sequencer not moving** | Check PLAY/STOP and CLOCK FOLLOW if using external MIDI clock. |
| **AUDIO STOP** | Reduce active voices/FX, inspect SYSTEM audio diagnostics and use HOME+PLAY rearm only if responsive. |
| **Manager cannot connect** | Use desktop Chrome/Edge, grant SysEx, use a data cable, choose the matching MIDI port and close other MIDI applications. |
| **`.etchpreset` import rejected** | Check file integrity, product identity, snapshot version, FM1 firmware version and stopped notes/transport. |
| **Firmware update fails** | Save the exact stage/error and logs. Do not retry blind flashing or erase storage as a workaround. |

---

## 15. Quick reference and screen directory

### Main page cycles

```text
SIMPLE SELECT:  SOUND → MORPH → FILTER → FX → LIBRARY
ADV SELECT:     SOUND → MORPH → FILTER → LIBRARY
ADV ARP:        ARP → ARP EDIT → CLOCK → RIFF LAB
ADV SEQ:        SEQ → NOTES → HARM → WAVE SEQ
ADV LFO MONO:   MOD → WAVE C FX → EVOLUTION → ORGANIC → SPECTRAL
                → LIVING → MOD MATRIX → WAVE AMP → PATCH BLEND → MOD
ADV LFO POLY:   MOD → WAVE C FX → MOD MATRIX → WAVE AMP → MOD
DRAW + FX:     WFX 1 → WFX 2 → SCULPT → TOOLS
```

The **ADVANCED screen catalog** provides direct access to all **64 existing page IDs** even when a particular page is not in the short button cycle. A MONO-only page will tell you when MONO is required.

### Complete V1.85 screen directory (64 pages)

| Category | Screens |
|---|---|
| **Core sound** | SOUND / PLAY, DRAW (PLAY editor), MORPH, MOD WAVE, FILTER, ENV, LIBRARY |
| **Wave editing / storage** | TOOLS, WFX 1, WFX 2, SCULPT, WAVE A FX, WAVE B FX, NAME, PRESETS, PATCH, PROJECT SAVE, PROJECTS |
| **Global FX** | FX overview, FX A, FX B, DRIVE, CHORUS, DELAY, REVERB, REVERSE DELAY, RING MOD, CHANNEL EQ, COMPRESSOR, TREMOLO, VIBRATO, PHASER, FLANGER, DECIMATOR, WAVEFOLDER, TAPE INSTABILITY, ROTARY SPEAKER, AUTO-WAH, FX BYPASS |
| **Modulation / expression** | WAVE C FX MOD, EVOLUTION, ORGANIC, SPECTRAL, LIVING, MOD MATRIX, PATCH BLEND, WAVE AMP, MOTION |
| **Rhythm / harmony** | ARP, ARP EDIT, RIFF LAB, SEQ, NOTES, GRID, HOLD, LOCKS, BAKE, CLOCK, WAVE SEQ, HARM, GUIDE, FIFTHS |
| **Performance / system** | GROUPS, PERFORMANCE, SYSTEM |

**DRAW is a mode of the PLAY page, not a separate numbered page.** The directory lists names by function rather than their internal numeric order. The actual source contains **64 page IDs**; boot prompts and destructive-action confirmations are separate UI states, not additional page IDs.

---

## 16. Technical information and credits

| Item | ETCH V1.85 |
|---|---|
| Hardware | M-VAVE FM1 / JieLi AC791N family |
| Firmware / package identity | `FM-1_1500` |
| Firmware label | `V184` with `S` or `A` workflow indication |
| Waveform editing | Three 128-point editable waves; DETAIL / SOFT / EDGE; 64 library slots |
| Named presets / projects | 32 / 4 slots |
| Polyphony | Up to eight voices, with load-dependent safeguards |
| Sequencer | Up to 256 steps; eight triggers per step |
| WAVE AMP | OFF / Wave A / Wave B / MORPH; audio-rate contour, 0–100% display |
| Project codec | `PJT32`, with backward-reading support for `PJT31` |
| Live sound snapshot | `.etchpreset` V1/V2; 1,044 bytes |
| UI | 240 × 240 RGB565, PHOSPHOR colour palette |
| DRAW update rhythm | 25 ms input checks; up to one wavetable compile per 70 ms during edits |
| Audio transients | 256-sample smooth note-on fade; rounded wavetable/voice crossfades |
| Safety status | Software-tested release candidate; not physically qualified |

**Credits and licensing.** ETCH is developed by **SikkenWorks**. Its FM1 low-level foundation, including portions of startup, drivers, LCD, USB-MIDI and OTA, incorporates work derived from **Felucca** by **Leo Kuroshita / Hügelton Instruments**. Relevant GPL and upstream notices are retained; ETCH's repository includes **GPL-3.0-only** licensing and separate third-party licensing information. JieLi SDK/toolchain and vendor components have their own terms. See the project `LICENSE`, `LICENSES/` and repository credits for authoritative details.

**ETCH · DRAW. MORPH. MODULATE.**  
**SikkenWorks · Born out of passion.**
