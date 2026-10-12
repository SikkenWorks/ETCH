#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from fm1_app_validate import APP_XIP, EXPECTED_ENTRY_STUB, parse_sections, parse_symbols, audit
from fm1_elf_to_app import place_sections


def main() -> int:
    syms_text = f"""
{APP_XIP:08x} g     F .text  00000020 _start
01c01000 g       .data  00000000 _data_start
01c01020 g       .data  00000000 _data_end
01c02000 g       .bss   00000000 _bss_start
01c02100 g       .bss   00000000 _bss_end
01c10000 g       .bss   00000000 _stack_top
"""
    secs_text = f"""
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00000020  {APP_XIP:08x}  {APP_XIP:08x}  00000100  2**2
  1 .data         00000020  01c01000  {APP_XIP+0x20:08x}  00000120  2**2
  2 .bss          00000100  01c02000  01c02000  00000140  2**2
"""
    syms=parse_symbols(syms_text); secs=parse_sections(secs_text)
    assert syms["_start"] == APP_XIP and len(secs)==3
    text = EXPECTED_ENTRY_STUB + bytes(range(28))
    data = bytes(range(32,64))
    app = place_sections([(secs[0], text),(secs[1], data)])
    report, errors, notes = audit(app, syms, secs)
    assert not errors, errors
    assert report["valid"] and report["entry_stub_ok"]
    assert app[:4] == EXPECTED_ENTRY_STUB and app[0x20:0x40] == data

    bad_syms=dict(syms); bad_syms["_start"] = APP_XIP + 4
    _, errors, _ = audit(app, bad_syms, secs)
    assert any("_start" in e for e in errors)

    bad_secs=[dict(x) for x in secs]; bad_secs[1]["vma"] = 0x02090000
    _, errors, _ = audit(app, syms, bad_secs)
    assert any(".data" in e for e in errors)

    overlap = dict(secs[1]); overlap["lma"] = APP_XIP + 0x10
    try:
        place_sections([(secs[0], text),(overlap,data)])
        raise AssertionError("overlap was not rejected")
    except ValueError as e:
        assert "overlap" in str(e)
    print("target app/link ABI tests: ok")
    return 0

if __name__ == "__main__": raise SystemExit(main())
