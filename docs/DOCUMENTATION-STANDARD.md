# ETCH documentation contract (all future versions)

This is a standing project requirement for **every ETCH firmware iteration**, including changes involving only firmware, UI, installers, sound engine, storage, MIDI or OTA.

## The three mandatory current files

1. `README.md`: timeless CURRENT product description, current how-to/controls, installation and recovery notes, credits/licenses, links to docs. **ABSOLUTELY NO CHANGELOG CONTENT** in README. No changelog heading, changelog link, release history, version-by-version bullet list, "What's new", "What changed", release-note fragments, or old-version feature announcements. Current version and actual backwards compatibility are allowed, but not chronology.
2. `USER_MANUAL.md`: full, clearly structured English user guide for the CURRENT firmware. All current commands, gestures, screens, restrictions, backup/recovery behaviors and newly added features must be integrated in their proper chapters. No accumulated version addenda. Keep every existing feature documented.
3. `CHEAT-SHEET.html`: offline, responsive, searchable **PHOSPHOR** card-style quick reference with current controls, shortcuts, how-to workflows, storage safety and troubleshooting. Maintain the current card/filter/search behavior. Text selection and right-click disabled with ONE JS script (not CSS) while permitting typing in the search field.

## Each release checklist

- Change version information across all three files.
- Confirm claims against the release's native source. Never make up keys, screen count, RAM numbers or hardware validation.
- Preserve all licenses and upstream credits in the README and source.
- Fix stale version/installer links and project codec numbers; keep package identity `FM-1_1500` unless expressly directed otherwise.
- Integrate every user-facing feature and control change into the main manual and matching cheat cards.
- Document observed software test results separately in a technical report, NOT the README.
- Run `python3 tools/check_docs_contract.py` and current firmware/installer tests.
- Provide the refreshed three documentation files in every iteration's output ZIP.
- Preserve the clean WebInstaller UI standard described in `docs/WEBINSTALLER-UI-STANDARD.md`.

This project file ensures the rule travels with the source to later builds or other conversations using the archive. Its presence does not by itself change settings or create guaranteed cross-chat memory.
