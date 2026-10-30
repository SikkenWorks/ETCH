# ETCH WebInstaller UI Standard (V1.85 onward)

**Permanent project rule for future ETCH releases.** Start from the V1.85 cleaned `WebInstaller.html`. Do not restore the old multiple yellow warning blocks or the outdated header text when regenerating the installer.

## Visual style

- Use the ETCH **PHOSPHOR** palette: deep aubergine `#15121C`, muted plum `#25202D`, warm ivory `#F2E9D8`, muted lavender `#A79AAE`, lime `#C7F36B`, coral `#FF7668`, lilac `#B39AFF` and subtle mauve grid lines `#403749`.
- One calm page containing **01 / Select Firmware** and **02 / Device & Progress**, plus a concise brand header and waveform preview.
- No loud yellow or amber warning cards, repeated version warnings, or long historical release text in the main install flow.
- Keep the package name, feature description, version, and hashes accurate for the **current** release; do not leave stale references such as V1.48 in the header or V1.78 storage changes as the V1.85 headline.
- Keep the UI responsive, with no horizontal overflow on mobile.

## Safety and compatibility (mandatory)

- Keep **Safety & compatibility** as a neutral, accessible collapsible `<details>` section; summarize critical experimental status, the need for a verified backup, device-side CHECK, protected header selection and limits of official stock restoration. The section is **not deleted** simply because it is collapsed.
- Keep an **always-visible and unchecked-by-default** explicit risk acknowledgement checkbox before update.
- Keep device CHECK, loader validation, post-flash identity verification and SHA-256 integrity checks exactly as implemented. A UI cleanup must not relax update acceptance or device safety.
- Display critical update failures in the main progress/status region; errors cannot be hidden inside `<details>`.
- Preserve Read-only Device Check, Download Diagnostic Log and a detailed progress indicator.
- Preserve Felucca/upstream credits and license notices.

## Browser interaction

- Keep selection and right-click prevention in **one JavaScript handler block**, not CSS and not multiple duplicated script tags. Permit normal keyboard editing in input/select/textarea fields.

## Release procedure

1. Copy the previous clean installer template to the new version's project.
2. Update the visible and dynamic version labels and the concise new-feature description.
3. Update both embedded `.fwsc` payloads and their SHA-256 values with the version's embed script; preserve the existing package identity `FM-1_1500`.
4. Run the current-version installer/OTA protocol tests and adapt the reusable `tools/check_webinstaller_ui.py` package paths/expected hashes for the new version.
5. Capture both desktop and mobile preview screens, check the collapsed/expanded safety panel, no layout overflow, and the clear status/error feedback.
6. Do not claim physical flash compatibility from these browser/source tests.

The versioned `tools/embed_v185_installer.py` works with this clean template: it only refreshes base64 firmware payloads and corresponding hashes, and does not reintroduce the previous warning cards.
