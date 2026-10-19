# ETCH V1.40 GROUPS — FM1 help and instructions

GROUPS is an eight-voice, three-part keyboard-split synthesizer: MAIN (the actively edited preset), GROUP 1 and GROUP 2 (saved presets). Notes outside the enabled Group zones play MAIN. Both extra Groups can sound concurrently and use **independent Wave A/B/C band-limited wavetable banks, morph, phase modulation depth/ratio, filters, amplifier ADSR, drive, velocity response, Living Waves controls and waveform pre-insert FX**. The global chorus, delay, reverb, compressor and other post-mix time-based effects remain a **single shared effects bus**.

## On your FM1

1. Save the presets you want for GROUP 1 and GROUP 2, including your hand-drawn waves.
2. Open **PROJECTS**, then press **FX** to open **GROUPS**.
3. K1 selects GROUP 1 or GROUP 2. K2 selects the saved preset number. K3 toggles that Group ON/OFF.
4. Press K4 to capture a range. Press the **first/lowest** key on the FM1 keyboard or external USB-MIDI keyboard, then a **strictly higher** key for END. Overlapping enabled Group ranges are rejected. Ranges include both endpoints.
5. Repeat for the second Group, if desired. Outside enabled Group ranges, the currently selected MAIN preset plays normally.
6. Go to PROJECTS and **SAVE** the project. The settings are stored in PJT29 alongside your original patches. Projects saved in PJT27/28 open with both Groups OFF.
7. To read on-device HELP, open **PROJECTS → REC**, then turn **K1** to cycle through the four help pages. Press **REC** to return.

## Performance

- Total polyphony: **eight simultaneous voices shared across all three parts**, not eight per part.
- Each incoming note is exclusively routed to one part, via either the FM1 keyboard or USB-MIDI. MIDI-channel identity and note-off/sustain behaviour are preserved.
- Group ranges must not overlap, and END must be above START. If a Group is OFF, MAIN covers that range again.
- A change to a Group preset, range, or waveform bank may be deferred until all old notes and their release tails have stopped. This protects the existing audio tables.
- Mono playback is automatically bypassed while at least one Group is enabled, so the three keyboard parts can play together.
- Post-mix time-based FX (chorus, delay, reverb and related effects) are shared. Per-preset waveform insert FX remain independent. This is **not three separate effects processors**.

## Web and firmware installation

- `simulator.html` redirects to the complete browser simulator (`sim/index.html`).
- `WebInstaller.html` contains the actual GROUPS test firmware and can transfer it over Web MIDI on desktop Chrome/Edge.
- The standalone `.fwsc` is located in `release/` and has the same bytes as the firmware embedded in the installer.
- Before flashing: back up your projects and retain a known-good ETCH installer and independent hardware recovery instructions. Package-format tests alone do **not** prove the firmware will boot successfully on your FM1.
