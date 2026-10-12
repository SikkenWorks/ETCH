#!/usr/bin/env python3
"""ETCH FM-1 package builder / verifier.

This is an original clean-room implementation of the documented JieLi/FM-1
container facts used by ETCH. It never talks to a device and never flashes.

Commands:
  info FILE.fwsc
  verify FILE.fwsc
  compare REFERENCE.fwsc CANDIDATE.fwsc
  lab-pack APP.bin OTA.bin OUT.fwsc.lab [--sdk PATH]
  release-pack APP.bin OTA.bin OUT.fwsc [--sdk PATH] [--proof FILE]
  extract FILE.fwsc OUTDIR
  lab-pack-from-stock APP.bin STOCK.fwsc OUT.fwsc.lab
  release-pack-from-stock APP.bin STOCK.fwsc OUT.fwsc [--proof FILE]

`release-pack` is intentionally gated by ETCH's R0 recovery proof record. The
lab command can exercise package generation without producing a `.fwsc` file.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
from typing import Iterable

APP_XIP = 0x02000120
APP_SLOT = 0x8DFBC
APP_AREA_FLASH = 0x4000
FLASH_PACKAGE_BYTES = 0x93000
KEY_MAC_FLASH = 0xFF000
DEFAULT_CHIP_KEY = 0x980F
DEFAULT_PRODUCT = "FM-1_923"
UFW_CHIP = b"AC791N"
# 30-byte JieLi SDK default flash-header payload for this FM-1 layout.
FLASH_HEADER_PAYLOAD = bytes.fromhex(
    "20022f104cc900f00f001010a4ff4e5d0b41e0cd781dc856417fb367cf9f"
)
EXPECTED_ENTRY_STUB = bytes.fromhex("04818000")
OTA_LOAD_ADDR = 0x01C0A800


def crc16(data: bytes | bytearray, seed: int = 0) -> int:
    c = seed & 0xFFFF
    for b in data:
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return c


def stream_xor(buf: bytearray, offset: int, size: int, key: int = 0xFFFF) -> None:
    for i in range(size):
        buf[offset + i] ^= key & 0xFF
        key = ((key << 1) ^ (0x1021 if key & 0x8000 else 0)) & 0xFFFF


def sfc_xor(buf: bytearray, offset: int, size: int, base: int, key: int) -> None:
    for rel in range(0, size, 32):
        block = min(32, size - rel)
        mixed = key ^ ((offset + rel - base) >> 2)
        stream_xor(buf, offset + rel, block, mixed)


def _name16(name: str | bytes, fill: int = 0xFF) -> bytes:
    if isinstance(name, bytes):
        if len(name) != 16:
            raise ValueError("raw JLFS name field must be exactly 16 bytes")
        return name
    raw = name.encode("ascii") + b"\0"
    if len(raw) > 16:
        raise ValueError(f"JLFS name too long: {name!r}")
    return raw + bytes([fill]) * (16 - len(raw))


def jlf_entry(data_crc: int, offset: int, size: int, flags: int, reserved: int,
              index: int, name: str | bytes, fill: int = 0xFF) -> bytes:
    body = struct.pack(
        "<HIIBBH16s", data_crc & 0xFFFF, offset & 0xFFFFFFFF,
        size & 0xFFFFFFFF, flags & 0xFF, reserved & 0xFF,
        index & 0xFFFF, _name16(name, fill)
    )
    return struct.pack("<H", crc16(body)) + body


def parse_jlf_entry(raw: bytes) -> dict:
    if len(raw) != 32:
        raise ValueError("JLFS entry must be 32 bytes")
    head_crc = struct.unpack_from("<H", raw, 0)[0]
    body = raw[2:]
    data_crc, offset, size, flags, reserved, index, name_raw = struct.unpack("<HIIBBH16s", body)
    name = name_raw.split(b"\0", 1)[0].decode("ascii", "replace")
    return {
        "header_crc": head_crc,
        "header_crc_ok": head_crc == crc16(body),
        "data_crc": data_crc,
        "offset": offset,
        "size": size,
        "flags": flags,
        "reserved": reserved,
        "index": index,
        "name": name,
        "name_raw_hex": name_raw.hex(),
    }


def chip_key_blob(key: int) -> bytes:
    left = bytearray([0x10] * 16)
    right = bytearray(16)
    threshold = 0x55
    for i in range(16):
        right[i] = left[15 - i] ^ (0x00 if (key >> i) & 1 else 0xFF)
    raw = bytes(left + right)
    if decode_chip_key(raw) != key:
        raise AssertionError("chip-key encoding self-check failed")
    return raw + struct.pack("<H", crc16(raw))


def decode_chip_key(raw32: bytes) -> int:
    if len(raw32) < 32:
        raise ValueError("chip-key blob too short")
    threshold = sum(raw32[:16]) & 0xFF
    threshold = 0xAA if threshold >= 0xE0 else 0x55 if threshold <= 0x10 else threshold
    out = 0
    for i in range(16):
        if (raw32[16 + i] ^ raw32[15 - i]) < threshold:
            out |= 1 << i
    return out


def _sdk_bytes(sdk: Path, rel: str) -> bytes:
    p = sdk / "cpu" / "wl82" / "tools" / rel
    if not p.is_file():
        raise FileNotFoundError(f"missing AC79 SDK file: {p}")
    return p.read_bytes()


def build_flash_image_from_assets(app: bytes, uboot: bytes, cfg_tool: bytes, eq_cfg: bytes,
                                  key: int = DEFAULT_CHIP_KEY, *,
                                  isd_config: bytes | None = None,
                                  flash_header_payload: bytes = FLASH_HEADER_PAYLOAD) -> bytes:
    if not app:
        raise ValueError("app.bin is empty")
    if len(app) > APP_SLOT:
        raise ValueError(f"app.bin is {len(app)} bytes; slot is {APP_SLOT}")
    if not uboot or not cfg_tool or not eq_cfg:
        raise ValueError("boot/config assets must be non-empty")

    if len(flash_header_payload) != 30:
        raise ValueError("flash header payload must be exactly 30 bytes")
    if isd_config is None:
        isd = chip_key_blob(key) + b"[ETCH]\r\n"
    else:
        isd = bytes(isd_config)
        if len(isd) < 32:
            raise ValueError("isd_config.ini is too small")
        if decode_chip_key(isd[:32]) != key:
            raise ValueError("isd_config.ini chip key does not match requested key")
    flash = bytearray(b"\xFF" * FLASH_PACKAGE_BYTES)
    uboot_off = 0xA0
    isd_off = uboot_off + len(uboot)
    if isd_off + len(isd) > APP_AREA_FLASH:
        raise ValueError("uboot + isd_config do not fit before app area")
    flash[uboot_off:uboot_off + len(uboot)] = uboot
    flash[isd_off:isd_off + len(isd)] = isd

    top_entries = b"".join((
        jlf_entry(crc16(uboot), uboot_off, len(uboot), 0x00, 0x00, 0, "uboot.boot"),
        jlf_entry(crc16(isd), isd_off, len(isd), 0x02, 0x80, 0, "isd_config.ini"),
        jlf_entry(0xFFFF, APP_AREA_FLASH, 0xFFFFFFFF, 0x81, 0xFF, 0, "app_dir_head"),
        jlf_entry(0xFFFF, KEY_MAC_FLASH, 0x1000, 0x12, 0x01, 0xFFFF, "key_mac"),
    ))
    header = struct.pack("<H", crc16(flash_header_payload)) + flash_header_payload
    top = bytearray(header + top_entries)
    if len(top) != 0xA0:
        raise AssertionError(f"top directory size is {len(top):#x}, expected 0xa0")
    for off in range(0, len(top), 32):
        stream_xor(top, off, 32, 0xFFFF)
    flash[:len(top)] = top

    padded_app = app + b"\xFF" * (APP_SLOT - len(app))
    app_block_size = 0x120 + APP_SLOT + len(cfg_tool)
    area = bytearray(b"\xFF" * app_block_size)
    area[0x120:0x120 + APP_SLOT] = padded_app
    area[0x120 + APP_SLOT:app_block_size] = cfg_tool

    descriptors = [
        jlf_entry(crc16(padded_app), 0x120, APP_SLOT, 0x82, 0xFF, 0, "app.bin"),
        jlf_entry(crc16(cfg_tool), 0x120 + APP_SLOT, len(cfg_tool), 0x82, 0xFF, 0, "cfg_tool.bin"),
        jlf_entry(0xFFFF, 0x93000, 352256, 0x12, 0x80, 0,
                  bytes.fromhex("564d00ffffffffffffffffff00800000")),
        jlf_entry(0xFFFF, 0, FLASH_PACKAGE_BYTES, 0x92, 0x82, 0,
                  bytes.fromhex("5052435400ffffffffffffff434f4445")),
        jlf_entry(0xFFFF, 0xE9000, 0x1000, 0x92, 0x80, 0,
                  bytes.fromhex("4254494600ffffffffffffff4155544f")),
        jlf_entry(0xFFFF, 0xEA000, 0x12000, 0x92, 0x81, 1, "USR"),
    ]
    joined = b"".join(descriptors)
    area[0x20:0x20 + len(joined)] = joined
    area[:0x20] = jlf_entry(crc16(area[0x20:]), APP_XIP, app_block_size, 0x83, 0xFF, 0, "app_area_head")

    eq_entry = jlf_entry(crc16(eq_cfg), 0x40, len(eq_cfg), 0x82, 0xFF, 1, "eq_cfg_hw.bin")
    cfg_body = eq_entry + eq_cfg
    cfg_dir = jlf_entry(crc16(cfg_body), 0x20, 0x20 + len(cfg_body), 0x83, 0xFF, 1, "cfg") + cfg_body
    region = bytearray(area + cfg_dir)
    if APP_AREA_FLASH + len(region) > FLASH_PACKAGE_BYTES:
        raise ValueError("app region exceeds packaged flash area")
    sfc_xor(region, 0, len(region), 0, key)
    flash[APP_AREA_FLASH:APP_AREA_FLASH + len(region)] = region
    return bytes(flash)


def build_flash_image(app: bytes, sdk: Path, key: int = DEFAULT_CHIP_KEY) -> bytes:
    return build_flash_image_from_assets(
        app,
        _sdk_bytes(sdk, "uboot.boot"),
        _sdk_bytes(sdk, "cfg_tool.bin"),
        _sdk_bytes(sdk, "cfg/eq_cfg_hw.bin"),
        key,
    )


def _ufw_entry(file_type: int, index: int, data: bytes, offset: int, name: str) -> bytes:
    e = bytearray(0x50)
    struct.pack_into("<HHHHIII", e, 0, file_type, index, crc16(data), 0, offset, len(data), (len(data) + 31) & ~31)
    nb = name.encode("ascii")
    if len(nb) > 15:
        raise ValueError("UFW file name too long")
    e[0x40:0x40 + len(nb)] = nb
    return bytes(e)


def build_fwsc(flash: bytes, ota: bytes, product: str = DEFAULT_PRODUCT) -> bytes:
    if not ota:
        raise ValueError("ota.bin is empty")
    if len(product) > 20:
        raise ValueError("product identity is limited to 20 characters")
    files = [(0, "flash.bin", flash), (100, "ota.bin", ota)]
    offset = 0x400
    entries: list[tuple[bytes, int, bytes]] = []
    for idx, (kind, name, data) in enumerate(files):
        offset = (offset + 0xFF) & ~0xFF
        entry = _ufw_entry(kind, idx, data, offset, name)
        entries.append((entry, offset, data))
        offset += len(data)
    total = offset

    header = bytearray(0x40)
    struct.pack_into("<IH", header, 4, total, len(entries))
    struct.pack_into("<HHH", header, 10, 0x0004, 0x0200, 0)
    header[16:16 + len(UFW_CHIP)] = UFW_CHIP
    encrypted_list = bytearray()
    for entry, _, _ in entries:
        enc = bytearray(entry)
        stream_xor(enc, 0, len(enc), 0xFFFF)
        encrypted_list += enc
    struct.pack_into("<H", header, 2, crc16(encrypted_list))
    struct.pack_into("<H", header, 0, crc16(header[2:]))
    stream_xor(header, 0, len(header), 0xFFFF)

    logical = bytearray(b"\xFF" * total)
    logical[:0x40] = header
    logical[0x40:0x40 + len(encrypted_list)] = encrypted_list
    for _, off, data in entries:
        logical[off:off + len(data)] = data

    raw = bytearray()
    for i in range(20):
        ch = product[i] if i < len(product) else None
        marker = ((ord(ch) + i + 1) & 0xFF) if ch else 0x7D
        if ch and marker == 0x7D:
            raise ValueError(f"product character {ch!r} at index {i} collides with padding marker")
        start = i * 0x2F
        raw += logical[start:start + 0x2F]
        raw.append(marker)
    raw += logical[20 * 0x2F:]
    return bytes(raw)


def decode_fwsc(raw: bytes) -> tuple[str, bytes]:
    if len(raw) < 20 * 0x30:
        raise ValueError("file is too small to be an FM-1 .fwsc")
    logical = bytearray()
    chars: list[str] = []
    identity_done = False
    for i in range(20):
        block = raw[i * 0x30:(i + 1) * 0x30]
        if len(block) != 0x30:
            raise ValueError("truncated identity blocks")
        logical += block[:0x2F]
        marker = block[0x2F]
        if marker == 0x7D:
            identity_done = True
        elif not identity_done:
            chars.append(chr((marker - i - 1) & 0xFF))
    logical += raw[20 * 0x30:]
    return "".join(chars), bytes(logical)


def parse_ufw(raw: bytes) -> dict:
    product, logical = decode_fwsc(raw)
    if len(logical) < 0x400:
        raise ValueError("logical UFW is too small")
    hdr = bytearray(logical[:0x40])
    stream_xor(hdr, 0, len(hdr), 0xFFFF)
    hdr_crc, list_crc = struct.unpack_from("<HH", hdr, 0)
    total, count = struct.unpack_from("<IH", hdr, 4)
    h4, h5, h6 = struct.unpack_from("<HHH", hdr, 10)
    chip = bytes(hdr[16:32]).split(b"\0", 1)[0].rstrip(b"\xff").decode("ascii", "replace")
    if count > 32:
        raise ValueError(f"implausible UFW file count {count}")
    list_raw = logical[0x40:0x40 + count * 0x50]
    entries = []
    for i in range(count):
        enc = bytearray(list_raw[i * 0x50:(i + 1) * 0x50])
        if len(enc) != 0x50:
            raise ValueError("truncated UFW entry list")
        stream_xor(enc, 0, len(enc), 0xFFFF)
        typ, idx, data_crc, zero, off, size, aligned = struct.unpack_from("<HHHHIII", enc, 0)
        name = bytes(enc[0x40:0x50]).split(b"\0", 1)[0].decode("ascii", "replace")
        data = logical[off:off + size]
        entries.append({
            "type": typ, "index": idx, "data_crc": data_crc, "zero": zero,
            "offset": off, "size": size, "aligned_size": aligned, "name": name,
            "data_crc_ok": len(data) == size and crc16(data) == data_crc,
            "sha256": hashlib.sha256(data).hexdigest() if len(data) == size else None,
            "data": data,
        })
    return {
        "product": product,
        "logical_size": len(logical),
        "declared_total": total,
        "file_count": count,
        "chip": chip,
        "header_fields": [h4, h5, h6],
        "header_crc_ok": hdr_crc == crc16(hdr[2:]),
        "list_crc_ok": list_crc == crc16(list_raw),
        "entries": entries,
    }


def parse_flash(flash: bytes) -> dict:
    if len(flash) < FLASH_PACKAGE_BYTES:
        raise ValueError(f"flash.bin is {len(flash):#x}; expected at least {FLASH_PACKAGE_BYTES:#x}")
    top = bytearray(flash[:0xA0])
    for off in range(0, len(top), 32):
        stream_xor(top, off, 32, 0xFFFF)
    head_crc = struct.unpack_from("<H", top, 0)[0]
    payload = bytes(top[2:32])
    top_entries = [parse_jlf_entry(bytes(top[0x20 + i * 0x20:0x40 + i * 0x20])) for i in range(4)]
    by_name = {e["name"]: e for e in top_entries}
    isd_e = by_name.get("isd_config.ini")
    key = None
    isd_crc_ok = None
    if isd_e and isd_e["offset"] + isd_e["size"] <= len(flash):
        isd = flash[isd_e["offset"]:isd_e["offset"] + isd_e["size"]]
        isd_crc_ok = crc16(isd) == isd_e["data_crc"]
        if len(isd) >= 32:
            key = decode_chip_key(isd[:32])
    app_area = None
    app_entry = None
    app_crc_ok = None
    if key is not None:
        enc = bytearray(flash[APP_AREA_FLASH:])
        sfc_xor(enc, 0, len(enc), 0, key)
        app_area = parse_jlf_entry(bytes(enc[:0x20]))
        if app_area["header_crc_ok"] and app_area["name"] == "app_area_head":
            app_entry = parse_jlf_entry(bytes(enc[0x20:0x40]))
            if app_entry["offset"] + app_entry["size"] <= len(enc):
                app_data = enc[app_entry["offset"]:app_entry["offset"] + app_entry["size"]]
                app_crc_ok = crc16(app_data) == app_entry["data_crc"]
    top_payloads = {}
    for e in top_entries:
        off, size = e["offset"], e["size"]
        if size != 0xFFFFFFFF and off + size <= len(flash):
            top_payloads[e["name"]] = {
                "bytes": size,
                "sha256": hashlib.sha256(flash[off:off + size]).hexdigest(),
            }
    return {
        "flash_header_crc_ok": head_crc == crc16(payload),
        "flash_header_payload_hex": payload.hex(),
        "top_entries": top_entries,
        "top_payloads": top_payloads,
        "isd_crc_ok": isd_crc_ok,
        "chip_key": key,
        "app_area": app_area,
        "app_entry": app_entry,
        "app_crc_ok": app_crc_ok,
    }


def extract_stock_assets(path: Path) -> dict:
    """Extract exact boot/config/update bytes from a user-supplied known-good FM-1 package.

    Nothing from the stock package is bundled with ETCH; the caller supplies it locally.
    """
    report, errors = verify_fwsc(path)
    if errors:
        raise ValueError("reference package failed verification: " + "; ".join(errors))
    pkg = parse_ufw(path.read_bytes())
    entries = {e["name"]: e for e in pkg["entries"]}
    flash = entries["flash.bin"]["data"]
    ota = entries["ota.bin"]["data"]

    top = bytearray(flash[:0xA0])
    for off in range(0, len(top), 32):
        stream_xor(top, off, 32, 0xFFFF)
    top_entries = [parse_jlf_entry(bytes(top[0x20+i*0x20:0x40+i*0x20])) for i in range(4)]
    by_name = {e["name"]: e for e in top_entries}
    def payload(e):
        off, size = e["offset"], e["size"]
        if size == 0xFFFFFFFF or off + size > len(flash):
            raise ValueError(f"invalid reference entry {e['name']}")
        return flash[off:off+size]
    uboot = payload(by_name["uboot.boot"])
    isd = payload(by_name["isd_config.ini"])
    flash_header_payload = bytes(top[2:32])
    if len(isd) < 32:
        raise ValueError("reference isd_config.ini too small")
    key = decode_chip_key(isd[:32])

    dec = bytearray(flash[APP_AREA_FLASH:])
    sfc_xor(dec, 0, len(dec), 0, key)
    area = parse_jlf_entry(bytes(dec[:0x20]))
    if not area["header_crc_ok"] or area["name"] != "app_area_head":
        raise ValueError("reference app_area_head invalid")
    descriptors = [parse_jlf_entry(bytes(dec[0x20+i*0x20:0x40+i*0x20])) for i in range(6)]
    desc = {e["name"]: e for e in descriptors if e["name"]}
    cfg_e = desc.get("cfg_tool.bin")
    if not cfg_e or cfg_e["offset"] + cfg_e["size"] > len(dec):
        raise ValueError("reference cfg_tool.bin not found")
    cfg_tool = bytes(dec[cfg_e["offset"]:cfg_e["offset"]+cfg_e["size"]])

    cfg_off = area["size"]
    if cfg_off + 0x40 > len(dec):
        raise ValueError("reference cfg directory outside decrypted area")
    cfg_dir = parse_jlf_entry(bytes(dec[cfg_off:cfg_off+0x20]))
    if not cfg_dir["header_crc_ok"] or cfg_dir["name"] != "cfg":
        raise ValueError("reference cfg directory not found at app-area boundary")
    eq_e = parse_jlf_entry(bytes(dec[cfg_off+0x20:cfg_off+0x40]))
    if not eq_e["header_crc_ok"] or eq_e["name"] != "eq_cfg_hw.bin":
        raise ValueError("reference eq_cfg_hw.bin entry not found")
    eq_start = cfg_off + eq_e["offset"]
    if eq_start + eq_e["size"] > len(dec):
        raise ValueError("reference eq_cfg_hw.bin outside decrypted area")
    eq_cfg = bytes(dec[eq_start:eq_start+eq_e["size"]])
    return {
        "product": report["product"], "key": key, "uboot": uboot,
        "isd_config": isd, "flash_header_payload": flash_header_payload,
        "cfg_tool": cfg_tool, "eq_cfg": eq_cfg, "ota": ota,
        "hashes": {
            "uboot.boot": hashlib.sha256(uboot).hexdigest(),
            "isd_config.ini": hashlib.sha256(isd).hexdigest(),
            "flash_header_payload": hashlib.sha256(flash_header_payload).hexdigest(),
            "cfg_tool.bin": hashlib.sha256(cfg_tool).hexdigest(),
            "eq_cfg_hw.bin": hashlib.sha256(eq_cfg).hexdigest(),
            "ota.bin": hashlib.sha256(ota).hexdigest(),
        },
    }


def validate_ota(ota: bytes) -> dict:
    out = {"bytes": len(ota), "outer_header_ok": False, "inner_header_ok": False, "name_ok": False}
    if len(ota) < 64:
        return out
    outer = parse_jlf_entry(ota[:32])
    inner = parse_jlf_entry(ota[32:64])
    out.update({"outer": outer, "inner": inner})
    out["outer_header_ok"] = outer["header_crc_ok"]
    out["inner_header_ok"] = inner["header_crc_ok"]
    out["name_ok"] = outer["name"] == "usb_hid_ota.bin" and inner["name"] == "usb_hid_ota.bin"
    if outer["offset"] == 0x20 and outer["size"] <= len(ota) - 32:
        body = ota[32:32 + outer["size"]]
        out["outer_data_crc_ok"] = crc16(body) == outer["data_crc"]
    else:
        out["outer_data_crc_ok"] = False
    # In the FM-1 OTA wrapper the inner JLFS offset field carries payload bytes
    # while its size field carries the RAM load address used by the loader.
    payload_bytes = inner.get("offset", 0)
    out["inner_load_ok"] = inner.get("size") == OTA_LOAD_ADDR
    from fm1_lz4 import decode_loader
    encoded = ota[64:32 + outer["size"]]
    out["inner_data_crc_ok"] = False
    out["inner_payload_bytes"] = payload_bytes
    # Distinguish exact raw payloads from length-framed LZ4 streams.
    if len(encoded) == payload_bytes and crc16(encoded) == inner["data_crc"]:
        out["inner_data_crc_ok"] = True
        out["encoding"] = "raw"
    else:
        try:
            payload = decode_loader(encoded, payload_bytes)
            out["inner_data_crc_ok"] = crc16(payload) == inner["data_crc"]
            out["encoding"] = "lz4-blocks"
        except (ValueError, IndexError) as exc:
            out["decode_error"] = str(exc)
    return out


def verify_fwsc(path: Path) -> tuple[dict, list[str]]:
    raw = path.read_bytes()
    pkg = parse_ufw(raw)
    errors: list[str] = []
    if pkg["chip"] != UFW_CHIP.decode(): errors.append(f"chip is {pkg['chip']!r}, expected AC791N")
    if not pkg["header_crc_ok"]: errors.append("UFW header CRC mismatch")
    if not pkg["list_crc_ok"]: errors.append("UFW file-list CRC mismatch")
    if pkg["declared_total"] != pkg["logical_size"]: errors.append("UFW declared size mismatch")
    occupied = []
    for e in pkg["entries"]:
        if not e["data_crc_ok"]: errors.append(f"{e['name']}: data CRC mismatch/truncation")
        if e["offset"] < 0x400: errors.append(f"{e['name']}: payload overlaps UFW directory")
        if e["offset"] + e["size"] > pkg["logical_size"]: errors.append(f"{e['name']}: payload range exceeds package")
        if e["aligned_size"] < e["size"] or e["aligned_size"] % 32: errors.append(f"{e['name']}: invalid aligned size")
        occupied.append((e["offset"], e["offset"] + e["size"], e["name"]))
    for i, a in enumerate(occupied):
        for b in occupied[i+1:]:
            if max(a[0], b[0]) < min(a[1], b[1]): errors.append(f"payload overlap: {a[2]} / {b[2]}")
    names = [e["name"] for e in pkg["entries"]]
    if names != ["flash.bin", "ota.bin"]: errors.append(f"UFW contents are {names}, expected flash.bin + ota.bin")
    flash_e = next((e for e in pkg["entries"] if e["name"] == "flash.bin"), None)
    ota_e = next((e for e in pkg["entries"] if e["name"] == "ota.bin"), None)
    flash_info = parse_flash(flash_e["data"]) if flash_e and flash_e["data_crc_ok"] else None
    ota_info = validate_ota(ota_e["data"]) if ota_e and ota_e["data_crc_ok"] else None
    if flash_info:
        if not flash_info["flash_header_crc_ok"]: errors.append("flash.bin top header CRC mismatch")
        if flash_info["chip_key"] != DEFAULT_CHIP_KEY: errors.append(f"chip key is {flash_info['chip_key']!r}, expected 0x980F")
        if flash_info["isd_crc_ok"] is False: errors.append("isd_config.ini CRC mismatch")
        if flash_info["app_area"] and flash_info["app_area"]["offset"] != APP_XIP:
            errors.append(f"app XIP is {flash_info['app_area']['offset']:#x}, expected {APP_XIP:#x}")
        ae = flash_info.get("app_entry")
        if ae:
            if ae.get("name") != "app.bin": errors.append("first app-area entry is not app.bin")
            if ae.get("offset") != 0x120: errors.append(f"app.bin relative offset is {ae.get('offset'):#x}, expected 0x120")
            if ae.get("size") != APP_SLOT: errors.append(f"app.bin slot is {ae.get('size')} bytes, expected {APP_SLOT}")
        if flash_info["app_crc_ok"] is False: errors.append("app.bin slot CRC mismatch")
    if ota_info:
        if not ota_info["outer_header_ok"] or not ota_info["inner_header_ok"] or not ota_info["name_ok"]:
            errors.append("ota.bin does not have the expected two-level usb_hid_ota.bin wrapper")
        if not ota_info.get("outer_data_crc_ok", False): errors.append("ota.bin outer payload CRC mismatch")
        if not ota_info.get("inner_data_crc_ok", False): errors.append("ota.bin inner payload CRC mismatch")
        if not ota_info.get("inner_load_ok", False): errors.append(f"ota.bin load address is not {OTA_LOAD_ADDR:#x}")
    report = {
        "path": str(path), "bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
        "product": pkg["product"], "chip": pkg["chip"],
        "header_crc_ok": pkg["header_crc_ok"], "list_crc_ok": pkg["list_crc_ok"],
        "entries": [{k: v for k, v in e.items() if k != "data"} for e in pkg["entries"]],
        "flash": flash_info, "ota": ota_info, "valid": not errors,
    }
    return report, errors


def validate_app_binary(app: bytes) -> list[str]:
    errors = []
    if not app: errors.append("app.bin is empty")
    if len(app) > APP_SLOT: errors.append(f"app.bin exceeds slot ({len(app)} > {APP_SLOT})")
    if app and app[:4] != EXPECTED_ENTRY_STUB:
        errors.append(f"app.bin entry stub {app[:4].hex()} != expected {EXPECTED_ENTRY_STUB.hex()}")
    if app and all(b == 0xFF for b in app[:min(len(app), 4096)]): errors.append("app.bin begins as erased flash")
    return errors


def validate_app_audit(app: Path, audit_path: Path) -> tuple[bool, list[str]]:
    errors: list[str] = []
    try:
        data = json.loads(audit_path.read_text())
    except Exception as e:
        return False, [f"cannot read app audit: {e}"]
    app_hash = hashlib.sha256(app.read_bytes()).hexdigest()
    if data.get("valid") is not True: errors.append("app audit valid is not true")
    if data.get("app_sha256") != app_hash: errors.append("app audit SHA-256 does not match app.bin")
    if data.get("entry_stub_ok") is not True: errors.append("app audit entry stub is not valid")
    if str(data.get("expected_start", "")).lower() != hex(APP_XIP).lower(): errors.append("app audit expected start does not match FM-1 contract")
    if data.get("app_bytes") != app.stat().st_size: errors.append("app audit byte count does not match app.bin")
    if data.get("errors") not in (None, []): errors.append("app audit contains errors")
    return not errors, errors


def validate_r0_proof(path: Path) -> tuple[bool, list[str]]:
    errors = []
    try:
        data = json.loads(path.read_text())
    except Exception as e:
        return False, [f"cannot read recovery proof: {e}"]
    if data.get("status") != "PROVEN": errors.append("status must be PROVEN")
    d1 = str(data.get("dump_1_sha256", "")); d2 = str(data.get("dump_2_sha256", ""))
    if len(d1) != 64 or any(c not in "0123456789abcdefABCDEF" for c in d1): errors.append("dump_1_sha256 is not a SHA-256")
    if d1.lower() != d2.lower(): errors.append("the two full-flash dump hashes are not identical")
    for key in ("restore_1_stock_verified", "restore_2_stock_verified", "usb_midi_verified_after_restore", "audio_verified_after_restore", "controls_verified_after_restore"):
        if data.get(key) is not True: errors.append(f"{key} is not true")
    rev = str(data.get("hardware_revision", "")).strip()
    if not rev or "record" in rev.lower(): errors.append("hardware_revision is missing")
    return not errors, errors


def compare_packages(a: Path, b: Path) -> dict:
    ra, ea = verify_fwsc(a); rb, eb = verify_fwsc(b)
    def shape(r):
        return {
            "chip": r.get("chip"),
            "entry_names": [e["name"] for e in r.get("entries", [])],
            "entry_types": [e["type"] for e in r.get("entries", [])],
            "entry_offsets": [e["offset"] for e in r.get("entries", [])],
            "flash_top_names": [e["name"] for e in (r.get("flash") or {}).get("top_entries", [])],
            "chip_key": (r.get("flash") or {}).get("chip_key"),
            "app_xip": ((r.get("flash") or {}).get("app_area") or {}).get("offset"),
            "uboot_sha256": ((r.get("flash") or {}).get("top_payloads") or {}).get("uboot.boot", {}).get("sha256"),
            "ota_shape": {
                "outer_offset": ((r.get("ota") or {}).get("outer") or {}).get("offset"),
                "outer_flags": ((r.get("ota") or {}).get("outer") or {}).get("flags"),
                "inner_load": ((r.get("ota") or {}).get("inner") or {}).get("size"),
            },
        }
    sa, sb = shape(ra), shape(rb)
    return {"reference": str(a), "candidate": str(b), "reference_valid": not ea, "candidate_valid": not eb,
            "same_container_shape": sa == sb, "reference_shape": sa, "candidate_shape": sb,
            "reference_errors": ea, "candidate_errors": eb}


def make_release_manifest(out: Path, app: Path, ota: Path | None, product: str, *,
                          ota_data: bytes | None = None, stock: Path | None = None) -> None:
    fw = out.read_bytes()
    if ota_data is None:
        if ota is None:
            raise ValueError("ota path or ota_data is required")
        ota_data = ota.read_bytes()
    manifest = {
        "name": out.name, "product": product, "target": "M-VAVE FM-1 / AC791N",
        "bytes": len(fw), "sha256": hashlib.sha256(fw).hexdigest(),
        "app_sha256": hashlib.sha256(app.read_bytes()).hexdigest(),
        "ota_sha256": hashlib.sha256(ota_data).hexdigest(),
        "warning": "Custom firmware; not affiliated with or endorsed by M-VAVE.",
    }
    if stock is not None:
        manifest["stock_reference"] = str(stock)
        manifest["stock_reference_sha256"] = hashlib.sha256(stock.read_bytes()).hexdigest()
    out.with_suffix(out.suffix + ".manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


def main(argv: Iterable[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("info", "verify"):
        p = sub.add_parser(name); p.add_argument("file", type=Path)
    p = sub.add_parser("compare"); p.add_argument("reference", type=Path); p.add_argument("candidate", type=Path)
    p = sub.add_parser("extract"); p.add_argument("file", type=Path); p.add_argument("outdir", type=Path)
    for name in ("lab-pack-from-stock", "release-pack-from-stock"):
        p = sub.add_parser(name)
        p.add_argument("app", type=Path); p.add_argument("stock", type=Path); p.add_argument("out", type=Path)
        p.add_argument("--product", default=DEFAULT_PRODUCT, help="ETCH package identity; boot/config/OTA assets still come from the verified stock/reference package")
        if name == "release-pack-from-stock":
            p.add_argument("--proof", type=Path, default=Path("R0_RECOVERY_PROVEN.json"))
            p.add_argument("--app-audit", type=Path, default=None)
    for name in ("lab-pack", "release-pack"):
        p = sub.add_parser(name)
        p.add_argument("app", type=Path); p.add_argument("ota", type=Path); p.add_argument("out", type=Path)
        p.add_argument("--sdk", type=Path, default=Path(os.environ.get("AC79_SDK", Path.home() / "fw-AC79_AIoT_SDK")))
        p.add_argument("--product", default=DEFAULT_PRODUCT)
        p.add_argument("--key", type=lambda s: int(s, 0), default=DEFAULT_CHIP_KEY)
        if name == "release-pack":
            p.add_argument("--proof", type=Path, default=Path("R0_RECOVERY_PROVEN.json"))
            p.add_argument("--app-audit", type=Path, default=None)
    args = ap.parse_args(list(argv) if argv is not None else None)

    if args.cmd in ("info", "verify"):
        report, errors = verify_fwsc(args.file)
        print(json.dumps(report, indent=2, default=lambda x: x.hex() if isinstance(x, bytes) else x))
        if args.cmd == "verify":
            for e in errors: print("FAIL:", e, file=sys.stderr)
            return 0 if not errors else 2
        return 0
    if args.cmd == "compare":
        report = compare_packages(args.reference, args.candidate)
        print(json.dumps(report, indent=2))
        return 0 if report["candidate_valid"] and report["same_container_shape"] else 2
    if args.cmd == "extract":
        report, errors = verify_fwsc(args.file)
        if errors:
            for e in errors: print("REFERENCE:", e, file=sys.stderr)
            return 2
        assets = extract_stock_assets(args.file)
        pkg = parse_ufw(args.file.read_bytes())
        entries = {e["name"]: e for e in pkg["entries"]}
        args.outdir.mkdir(parents=True, exist_ok=True)
        (args.outdir / "flash.bin").write_bytes(entries["flash.bin"]["data"])
        (args.outdir / "ota.bin").write_bytes(entries["ota.bin"]["data"])
        for name, data in (("uboot.boot", assets["uboot"]), ("isd_config.ini", assets["isd_config"]), ("cfg_tool.bin", assets["cfg_tool"]), ("eq_cfg_hw.bin", assets["eq_cfg"])):
            (args.outdir / name).write_bytes(data)
        (args.outdir / "EXTRACT.json").write_text(json.dumps({"source": str(args.file), "product": assets["product"], "key": hex(assets["key"]), "hashes": assets["hashes"]}, indent=2) + "\n")
        print(f"extracted verified local reference assets to {args.outdir}")
        return 0

    stock_mode = args.cmd in ("lab-pack-from-stock", "release-pack-from-stock")
    if args.cmd in ("lab-pack", "lab-pack-from-stock") and args.out.suffix.lower() == ".fwsc":
        ap.error("lab-pack refuses the .fwsc extension; use .fwsc.lab")
    if args.cmd in ("release-pack", "release-pack-from-stock") and args.out.suffix.lower() != ".fwsc":
        ap.error("release-pack output must end in .fwsc")
    if args.cmd in ("release-pack", "release-pack-from-stock"):
        ok, errs = validate_r0_proof(args.proof)
        if not ok:
            for e in errs: print("R0 GATE:", e, file=sys.stderr)
            print("Refusing to create a flashable .fwsc before ETCH R0 proof passes.", file=sys.stderr)
            return 3
        audit_path = args.app_audit or Path(str(args.app) + ".audit.json")
        audit_ok, audit_errs = validate_app_audit(args.app, audit_path)
        if not audit_ok:
            for e in audit_errs: print("APP AUDIT:", e, file=sys.stderr)
            print("Refusing release packaging without a matching successful app.bin audit sidecar.", file=sys.stderr)
            return 3

    if stock_mode:
        app = args.app.read_bytes()
        app_errors = validate_app_binary(app)
        if app_errors:
            for e in app_errors: print("APP:", e, file=sys.stderr)
            return 2
        try:
            assets = extract_stock_assets(args.stock)
        except Exception as e:
            print("STOCK:", e, file=sys.stderr)
            return 2
        product = args.product
        if product == 'FM-1_994' and assets['product'] == 'FM-1_989':
            raise SystemExit('V0.94 upgrade from V0.89 MUST use fm1_v094_compat.py to preserve the byte-exact 16-KiB installed boot header')
        flash = build_flash_image_from_assets(
            app, assets["uboot"], assets["cfg_tool"], assets["eq_cfg"], assets["key"],
            isd_config=assets["isd_config"], flash_header_payload=assets["flash_header_payload"]
        )
        package = build_fwsc(flash, assets["ota"], product)
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_bytes(package)
        report, errors = verify_fwsc(args.out)
        if errors:
            args.out.unlink(missing_ok=True)
            for e in errors: print("PACKAGE:", e, file=sys.stderr)
            return 2
        make_release_manifest(args.out, args.app, None, product, ota_data=assets["ota"], stock=args.stock)
        side = args.out.with_suffix(args.out.suffix + ".stock-assets.json")
        side.write_text(json.dumps({"stock": str(args.stock), "stock_product": assets["product"], "output_product": product, "stock_asset_hashes": assets["hashes"]}, indent=2) + "\n")
        print(f"wrote {args.out} ({len(package)} bytes) identity={product} sha256={report['sha256']}")
        print("reused boot/config/OTA bytes from the user-supplied verified stock package")
        if args.cmd == "lab-pack-from-stock": print("LAB ONLY: extension prevents accidental use as a normal .fwsc package.")
        return 0

    if args.cmd == "lab-pack" and args.out.suffix.lower() == ".fwsc":
        ap.error("lab-pack refuses the .fwsc extension; use .fwsc.lab")
    app = args.app.read_bytes(); ota = args.ota.read_bytes()
    app_errors = validate_app_binary(app)
    if app_errors:
        for e in app_errors: print("APP:", e, file=sys.stderr)
        return 2
    ota_info = validate_ota(ota)
    if not (ota_info["outer_header_ok"] and ota_info["inner_header_ok"] and ota_info["name_ok"] and ota_info.get("outer_data_crc_ok") and ota_info.get("inner_data_crc_ok") and ota_info.get("inner_load_ok")):
        print("OTA: wrapper validation failed", file=sys.stderr)
        return 2
    flash = build_flash_image(app, args.sdk, args.key)
    package = build_fwsc(flash, ota, args.product)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(package)
    report, errors = verify_fwsc(args.out)
    if errors:
        args.out.unlink(missing_ok=True)
        for e in errors: print("PACKAGE:", e, file=sys.stderr)
        return 2
    make_release_manifest(args.out, args.app, args.ota, args.product)
    print(f"wrote {args.out} ({len(package)} bytes) identity={args.product} sha256={report['sha256']}")
    if args.cmd == "lab-pack": print("LAB ONLY: extension prevents accidental use as a normal .fwsc package.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
