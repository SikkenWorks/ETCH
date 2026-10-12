#!/usr/bin/env python3
"""Read-only validator for an ETCH FM-1 app.bin / ELF link result.

This tool does not flash hardware. It checks the ABI/layout facts that a linked
AC791N/WL82 application must satisfy before ETCH will let the package stage use it.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from fm1_elf_sections import elf_sections

APP_XIP = 0x02000120
APP_SLOT = 0x8DFBC
RAM_START = 0x01C00000
RAM_SAFE_END = 0x01C7F000  # upper boot-info/vector/mailbox area is kept out of ETCH allocations
EXPECTED_ENTRY_STUB = bytes.fromhex("04818000")

SYM_RE = re.compile(r"^\s*([0-9a-fA-F]+)\s+.*\s([A-Za-z_.$][\w.$]*)\s*$")


def parse_symbols(text: str) -> dict[str, int]:
    out: dict[str, int] = {}
    for line in text.splitlines():
        m = SYM_RE.match(line)
        if m:
            out[m.group(2)] = int(m.group(1), 16)
    return out


def parse_sections(text: str) -> list[dict]:
    """Parse GNU/JieLi objdump -h style section rows.

    Expected columns after index/name are size, VMA, LMA, file offset, alignment.
    We intentionally ignore flags continuation lines.
    """
    out = []
    for line in text.splitlines():
        p = line.split()
        if len(p) < 7 or not p[0].isdigit() or not p[1].startswith("."):
            continue
        try:
            size = int(p[2], 16); vma = int(p[3], 16); lma = int(p[4], 16); off = int(p[5], 16)
        except ValueError:
            continue
        out.append({"name": p[1], "size": size, "vma": vma, "lma": lma, "file_offset": off})
    return out


def range_inside(addr: int, size: int, lo: int, hi: int) -> bool:
    return size >= 0 and lo <= addr <= hi and addr + size <= hi


def audit(app: bytes, symbols: dict[str, int] | None = None, sections: list[dict] | None = None) -> tuple[dict, list[str], list[str]]:
    errors: list[str] = []
    notes: list[str] = []
    if not app:
        errors.append("app.bin is empty")
    if len(app) > APP_SLOT:
        errors.append(f"app.bin is {len(app)} bytes; slot limit is {APP_SLOT}")
    if app and app[:4] != EXPECTED_ENTRY_STUB:
        errors.append(f"entry stub {app[:4].hex()} != {EXPECTED_ENTRY_STUB.hex()}")
    if app and all(b in (0x00, 0xFF) for b in app[:min(256, len(app))]):
        errors.append("first 256 app bytes look blank/erased")

    if symbols is not None:
        start = symbols.get("_start")
        if start is None:
            errors.append("_start symbol is missing")
        elif start != APP_XIP:
            errors.append(f"_start={start:#010x}; expected {APP_XIP:#010x}")
        else:
            notes.append(f"_start={start:#010x}")
        for a, b in (("_bss_start", "_bss_end"), ("_data_start", "_data_end"), ("_rt_start", "_rt_end")):
            if a in symbols and b in symbols:
                lo, hi = symbols[a], symbols[b]
                if hi < lo:
                    errors.append(f"{b} precedes {a}")
                elif not range_inside(lo, hi-lo, RAM_START, RAM_SAFE_END):
                    errors.append(f"{a}..{b} lies outside ETCH safe RAM window")
        for stack_name in ("_stack_top", "_sp", "__stack_top"):
            if stack_name in symbols and not (RAM_START <= symbols[stack_name] < RAM_SAFE_END):
                errors.append(f"{stack_name}={symbols[stack_name]:#x} outside ETCH safe RAM window")

    if sections is not None:
        for sec in sections:
            name, size, vma, lma = sec["name"], sec["size"], sec["vma"], sec["lma"]
            if size == 0:
                continue
            if name.startswith((".text", ".rodata", ".init", ".vectors")):
                if not range_inside(vma, size, APP_XIP, APP_XIP + APP_SLOT):
                    errors.append(f"{name} VMA {vma:#x}+{size:#x} outside app XIP slot")
            elif name.startswith((".data", ".bss", ".ram_text", ".noinit", ".pool")):
                if not range_inside(vma, size, RAM_START, RAM_SAFE_END):
                    errors.append(f"{name} VMA {vma:#x}+{size:#x} outside ETCH safe RAM")
                if name != ".bss" and lma >= APP_XIP and not range_inside(lma, size, APP_XIP, APP_XIP + APP_SLOT):
                    errors.append(f"{name} LMA {lma:#x}+{size:#x} outside app XIP slot")
        xip = [s for s in sections if s["size"] and APP_XIP <= s["vma"] < APP_XIP + APP_SLOT]
        if not xip:
            errors.append("no non-empty XIP section starts in the FM-1 app slot")

    report = {
        "target": "M-VAVE FM-1 / JieLi AC791N-WL82 / pi32v2",
        "app_bytes": len(app),
        "app_sha256": hashlib.sha256(app).hexdigest(),
        "app_slot_bytes": APP_SLOT,
        "headroom_bytes": APP_SLOT - len(app) if len(app) <= APP_SLOT else 0,
        "expected_start": hex(APP_XIP),
        "entry_stub_ok": bool(app) and app[:4] == EXPECTED_ENTRY_STUB,
        "safe_ram": [hex(RAM_START), hex(RAM_SAFE_END)],
        "symbol_count": len(symbols or {}),
        "sections": sections or [],
        "valid": not errors,
    }
    return report, errors, notes


def run_objdump(elf: Path, toolchain: Path) -> tuple[str, str]:
    objdump = toolchain / "common/bin/objdump"
    if not objdump.is_file():
        raise FileNotFoundError(f"missing JieLi objdump: {objdump}")
    syms = subprocess.run([str(objdump), "-t", str(elf)], check=True, capture_output=True, text=True).stdout
    secs = subprocess.run([str(objdump), "-h", str(elf)], check=True, capture_output=True, text=True).stdout
    return syms, secs


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("app", type=Path)
    ap.add_argument("--symbols", type=Path, help="objdump -t text")
    ap.add_argument("--sections", type=Path, help="objdump -h text")
    ap.add_argument("--elf", type=Path, help="ELF to inspect with JieLi objdump")
    ap.add_argument("--toolchain", type=Path, default=Path(os.environ.get("JIELI_TOOLCHAIN", Path.home()/".jieli/toolchain")))
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--report", type=Path, help="write a machine-readable audit sidecar")
    args = ap.parse_args()
    symbols = sections = None
    if args.elf:
        try:
            st, ht = run_objdump(args.elf, args.toolchain)
        except Exception as e:
            print(f"ELF audit: {e}", file=sys.stderr); return 2
        symbols, sections = parse_symbols(st), elf_sections(args.elf)
    else:
        if args.symbols: symbols = parse_symbols(args.symbols.read_text(errors="replace"))
        if args.sections: sections = parse_sections(args.sections.read_text(errors="replace"))
    report, errors, notes = audit(args.app.read_bytes(), symbols, sections)
    full_report = dict(report)
    full_report["errors"] = errors
    full_report["notes"] = notes
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(full_report, indent=2) + "\n")
    if args.json:
        print(json.dumps(full_report, indent=2))
    else:
        print(f"ETCH FM-1 app audit: {args.app}")
        for n in notes: print(" OK ", n)
        print(f" app: {report['app_bytes']} B / {APP_SLOT} B, headroom {report['headroom_bytes']} B")
        print(" entry stub:", "OK" if report["entry_stub_ok"] else "FAIL")
        for e in errors: print(" FAIL", e)
    return 0 if not errors else 2

if __name__ == "__main__":
    raise SystemExit(main())
