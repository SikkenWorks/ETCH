#!/usr/bin/env python3
"""Convert a verified JieLi pi32v2 ELF into the flat FM-1 app.bin image.

Only sections whose load memory address (LMA) lies inside the documented FM-1
application XIP slot are emitted. RAM-only BSS/noinit sections are intentionally
not emitted. This keeps VMA and load-image placement separate and auditable.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import sys
from fm1_elf_sections import elf_sections
from fm1_app_validate import APP_XIP, APP_SLOT, parse_sections, parse_symbols, audit


def tool(toolchain: Path, rel: str) -> Path:
    p = toolchain / rel
    if not p.is_file():
        raise FileNotFoundError(f"missing JieLi tool {p}")
    return p


def read_elf_metadata(elf: Path, tc: Path) -> tuple[str, str]:
    objdump = tool(tc, "common/bin/objdump")
    syms = subprocess.run([str(objdump), "-t", str(elf)], check=True, capture_output=True, text=True).stdout
    hdr = subprocess.run([str(objdump), "-h", str(elf)], check=True, capture_output=True, text=True).stdout
    return syms, hdr


def place_sections(section_blobs: list[tuple[dict, bytes]]) -> bytes:
    end = 0
    for sec, blob in section_blobs:
        if len(blob) != sec["size"]:
            raise ValueError(f"{sec['name']}: objcopy returned {len(blob)} B; section says {sec['size']} B")
        off = sec["lma"] - APP_XIP
        if off < 0 or off + len(blob) > APP_SLOT:
            raise ValueError(f"{sec['name']}: LMA {sec['lma']:#x} outside app slot")
        end = max(end, off + len(blob))
    image = bytearray(b"\xff" * end)
    occupied: list[tuple[int,int,str]] = []
    for sec, blob in sorted(section_blobs, key=lambda x: x[0]["lma"]):
        off = sec["lma"] - APP_XIP
        rng = (off, off + len(blob), sec["name"])
        for a,b,n in occupied:
            if max(a,rng[0]) < min(b,rng[1]):
                raise ValueError(f"load-image overlap: {n} / {sec['name']}")
        image[off:off+len(blob)] = blob
        occupied.append(rng)
    while len(image) % 4:
        image.append(0xFF)
    return bytes(image)


def build(elf: Path, out: Path, tc: Path) -> tuple[bytes, dict, list[str], list[str]]:
    syms_text, sec_text = read_elf_metadata(elf, tc)
    syms, secs = parse_symbols(syms_text), elf_sections(elf)
    objcopy = tool(tc, "common/bin/objcopy")
    load_secs = [s for s in secs if s["size"] and APP_XIP <= s["lma"] < APP_XIP + APP_SLOT and not s.get("nobits",False)]
    if not load_secs:
        raise ValueError("ELF has no loadable sections in the FM-1 app slot")
    blobs = []
    with tempfile.TemporaryDirectory(prefix="etch-fm1-") as td:
        for i, sec in enumerate(load_secs):
            tmp = Path(td) / f"{i:02d}.bin"
            subprocess.run([str(objcopy), "-O", "binary", "-j", sec["name"], str(elf), str(tmp)], check=True)
            data = tmp.read_bytes() if tmp.exists() else b""
            blobs.append((sec, data))
    image = place_sections(blobs)
    report, errors, notes = audit(image, syms, secs)
    if errors:
        raise ValueError("app audit failed: " + "; ".join(errors))
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(image)
    return image, report, errors, notes


def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("elf", type=Path); ap.add_argument("out", type=Path)
    ap.add_argument("--toolchain", type=Path, default=Path(os.environ.get("JIELI_TOOLCHAIN", Path.home()/".jieli/toolchain")))
    args=ap.parse_args()
    try:
        image, report, _, notes = build(args.elf, args.out, args.toolchain)
    except Exception as e:
        print(f"fm1_elf_to_app: {e}", file=sys.stderr); return 2
    print(f"wrote {args.out}: {len(image)} B, headroom {APP_SLOT-len(image)} B")
    for n in notes: print(" OK", n)
    return 0

if __name__ == "__main__": raise SystemExit(main())
