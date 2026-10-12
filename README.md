# ETCH

**DRAW. MORPH. MODULATE.**

ETCH is my custom firmware for the M-VAVE FM1, built under SikkenWorks.

I wanted to turn the FM1 into something a little different. Rather than starting with a standard FM algorithm or the usual fixed oscillator shapes, I wanted to actually *draw* the sound. Take a waveform, change it with the hardware controls, save it, blend it with another one, and use a third waveform to modulate the result.

That's basically where ETCH started. It has grown quite a bit since then, but the idea is still the same: make the waveform itself the instrument.

## How it works

Start by drawing or editing a single-cycle waveform directly on the FM1. Save it in the waveform library. You can use those saved waves in different patches, so you don't have to start from scratch every time.

Wave A and Wave B are read at the same oscillator phase. **MORPH** interpolates between their shapes to create a single carrier. Then **Wave C**, another waveform you've drawn or saved, can modulate that carrier. There's an envelope, a filter, drive, per-wave processing and global effects to shape the rest of the sound.

There is a lot more in there too: eight-voice polyphony, a sequencer, an arpeggiator, scale-aware harmony tools, Motion Recording, Wave Genetics and Wave Evolution. **ARP mode 7 (RIFF)** generates little melodic ideas within the selected key and scale; turn K4 to make another one.

When you're editing Wave A, Wave B or Wave C, ETCH lets you hear that individual waveform instead of the complete morphed patch. Wave A FX and Wave B FX also have their own audition path. Leave the editor and you're back to the complete synth.

### A few controls

- **HOME:** short press returns to PLAY. Hold it for **1.5 seconds** to open the named PRESETS browser.
- **PRESETS encoder:** scrolls through saved, named presets **only in the PRESETS browser**. Outside that menu, it does nothing. K1 selects save slots in the SAVE editors.
- **EDIT / DRAW:** use the four directional controls to draw a waveform on the FM1 itself.
- **LFO:** visit Wave C modulation, Wave C FX and EVOLUTION.
- **ARP:** choose an arpeggiator mode, including RIFF, then use K4 in RIFF mode to generate a new pattern.
- **HOME (short) from PRESETS:** exit back to PLAY.

The screen is designed around the waveforms, not just rows of numbers. I wanted it to feel like a tiny oscilloscope that you can actually play.

## Installing and building

The `ETCH-WebInstaller-V1.20.html` file contains the experimental firmware package and a USB-MIDI updater. Open it in a compatible desktop browser on a secure origin. It verifies the package locally, checks the expected firmware identity and protected header, and only reports success after the FM1 reconnects with the new identity. It also offers a stock V15 restore route that requires you to supply the official firmware file yourself.

**Important:** V1.20 is an experimental build, not a physically qualified release. Software build and host tests do **not** prove that flashing is safe. The FM1 has a single application bank. Without a tested independent ROM-level backup and restore route, a bad update may leave the instrument unusable. Keep backups and do not flash blindly, especially if you only have one FM1.

Building the device binary requires the JieLi pi32v2 toolchain:

```bash
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20250324.1
python3 firmware/fm1-native/build.py --storage --updater
```

To rebuild the experimental firmware container from a previously verified V1.19 package, use `python3 tools/pack_lab_preserve_head.py APP.bin REFERENCE-V1.19.fwsc.lab OUTPUT-V1.20.fwsc.lab`. The script verifies the reference identity and requires an unchanged protected boot header and OTA payload. This is an offline packaging check, not proof of safe flashing.

The build script also enables the existing multi-project option when storage is selected. To run the main host checks:

```bash
python3 tools/audit_felucca_core.py
python3 tests/test_v120_preset_policy.py
gcc -std=c11 -O2 tests/test_native_preset_browser.c -o /tmp/etch_presets && /tmp/etch_presets
python3 tools/run_native_tests.py
npm test
cmake -S . -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

The source code is here so you can inspect it, build it and change it. No proprietary M-VAVE stock firmware is included.

## About Felucca, and credits

I built ETCH as its own waveform-drawing and morphing synthesizer, but I definitely didn't start from nothing.

**Felucca, by Leo Kuroshita / Hügelton Instruments, provided the essential FM1 hardware and firmware foundation.** ETCH reuses Felucca-derived startup/boot code, low-level FM1 drivers, interrupt handling, the LCD hardware interface and parts of the USB-MIDI / OTA update infrastructure. The browser installer's OTA transport is also based on Felucca's updater. Some USB, audio and storage integration code has since been changed for ETCH, so compatibility and hardware stability must be tested rather than assumed.

The ETCH-specific sound engine, waveform editing workflow, A/B shape morphing, Wave C modulation and the additional sound-design features are developed around that foundation. The original names and copyright notices remain in the relevant source files. The update loader's protocol identifier is intentionally unchanged for compatibility, even though the visible installer and its logs use ETCH branding.

Felucca is published under the **GNU GPL-3.0-only** license. ETCH is distributed under **GPL-3.0-only** as a derivative work, with corresponding source code and licensing information included. The full license is in [`LICENSE`](LICENSE); upstream licensing details, including other component credits, are preserved in `LICENSES/FELUCCA-LICENSING.txt` and the files in `firmware/fm1-native/LICENSES/`.

Huge credit to [Felucca](https://github.com/hugelton/Felucca), [Leo Kuroshita / Hügelton Instruments](https://github.com/hugelton), and everyone involved in reverse-engineering the FM1. That work made this project possible.

ETCH is an independent SikkenWorks project. It isn't affiliated with or endorsed by M-VAVE or Hügelton Instruments.

**SikkenWorks · Born out of passion.**
