<p align="center">
  <img src="/ETCH-GH-banner.png" width="400" alt="ETCH Logo">
</p>

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

The `ETCH-WebInstaller-V1.20.html` file contains an experimental firmware package and a USB-MIDI updater. Open it in desktop Chrome or Edge over HTTPS or localhost, with Web MIDI/SysEx permission enabled. It checks the package, expected firmware identity and protected header, and uses the FM1's existing update protocol. It also provides a stock V15 restore option, but you have to supply the official firmware file yourself.

**Important:** V1.20 is an experimental build, not a physically qualified release. A successful software build or host test does **not** prove that flashing is safe. The FM1 has a single application bank. Without a tested, independent ROM-level backup and restore route, a bad update may leave the instrument unusable. Keep backups and don't flash blindly, especially if you only have one FM1.

Building the device binary requires the JieLi pi32v2 toolchain. The toolchain archive I used is named `jieli-linux-toolchains-20250805.1.tar.xz`, but the folder inside it is named `jieli-linux-toolchains-20250324.1`. Point `JIELI_TOOLCHAIN` to that **extracted folder**, which contains `pi32v2/` and `common/`.

```bash
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20250324.1
python3 firmware/fm1-native/build.py --storage --updater
```

The `--storage --updater` options enable experimental flash-storage and firmware-update functionality. They are not a substitute for hardware recovery testing.

To rebuild the experimental `.fwsc.lab` container, you also need a separately obtained and verified **V1.19** reference package. It is not included in this repository. From the project root:

```bash
python3 tools/pack_lab_preserve_head.py \
  build/fm1-native-storage-updater-p05/app.bin \
  /path/to/verified/REFERENCE-V1.19.fwsc.lab \
  ETCH-FM1-V1.20-EXPERIMENTAL.fwsc.lab
```

That script checks the firmware package and preserves the verified reference's protected 16 KB boot header and OTA payload. These are offline checks, **not proof of safe installation**. The build script also enables the existing multi-project option when storage is selected.

To run the main host checks:

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

The source code is here so you can inspect it, build it and change it. The official M-VAVE V15 firmware is not included.

## About Felucca, and credits

I built ETCH around its own waveform-drawing and morphing synthesizer, but I definitely didn't start from nothing.

**[Felucca](https://github.com/hugelton/Felucca), by Leo Kuroshita (@kurogedelic) / Hügelton Instruments, provided the essential FM1 hardware and firmware foundation.** ETCH uses Felucca-derived startup and boot code, low-level hardware drivers, interrupt handling, the LCD interface, and parts of the audio, USB-MIDI and OTA update infrastructure. Some of those parts have been modified for ETCH, so compatibility and hardware stability shouldn't be assumed just because they work in Felucca.

The **ETCH web installer** is also based in part on Felucca's USB-MIDI firmware updater and OTA transport implementation. I've adapted its interface, status messages and logs for ETCH, while preserving the underlying update protocol and the original attribution and licensing in the source.

ETCH's waveform editor, A/B shape-morphing and Wave C workflow, along with the additional ETCH sound-design features, have been developed on top of that foundation. The original Felucca copyright and GPL notices remain in the relevant source files.

### Other upstream code and references

There is another technical part worth crediting: **JieLi Technology**. ETCH uses the JieLi pi32v2 toolchain and a Felucca-compatible firmware packaging and boot/update format. The upstream Felucca licensing notes identify the AC79 SDK boot and configuration components used in firmware packages as **Apache-2.0** material from JieLi. Those components aren't original ETCH code; their separate licensing and provenance matter when distributing firmware binaries.

Felucca itself also credits projects including **[X0X](https://github.com/charlesvestal/fm1-x0x)** for its browser-emulator work, and **[DaisySP](https://github.com/electro-smith/DaisySP)**, **[Rings](https://github.com/pichenettes/eurorack)** and **[msfa/Dexed](https://github.com/asb2m10/dexed)** for synthesis code used in other Felucca engines. Those credits are part of Felucca's upstream history. I haven't identified their browser-emulator or synthesis source modules as directly included in the ETCH V1.20 source tree, so I don't want to imply that ETCH uses those engines.

The copied upstream licensing inventory in `LICENSES/FELUCCA-LICENSING.txt` is retained for attribution and reference. It describes **Felucca's** broader source tree, not necessarily the set of components shipped with ETCH. Additional upstream license notices are preserved in `firmware/fm1-native/LICENSES/`.

ETCH is distributed under **GNU GPL-3.0-only** as a Felucca-derived work. The full project license is in [`LICENSE`](LICENSE), and the corresponding ETCH source is included. Components with separate licenses retain their original notices where applicable.

Huge credit to [Felucca](https://github.com/hugelton/Felucca), [Leo Kuroshita / Hügelton Instruments](https://github.com/hugelton), and everyone involved in reverse-engineering the FM1. That work made this project possible.

ETCH is an independent SikkenWorks project. It isn't affiliated with or endorsed by M-VAVE, JieLi Technology or Hügelton Instruments.

**SikkenWorks · Born out of passion.**
