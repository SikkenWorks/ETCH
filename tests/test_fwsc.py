#!/usr/bin/env python3
from __future__ import annotations
import json
import tempfile
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import fm1_fwsc as fw


def ota_fixture(payload: bytes = b"ETCH OTA TEST") -> bytes:
    # Minimal structurally valid two-level wrapper for package-format tests only.
    inner = fw.jlf_entry(fw.crc16(payload), len(payload), 0x01C0A800, 0, 0, 0, "usb_hid_ota.bin")
    body = inner + payload
    outer = fw.jlf_entry(fw.crc16(body), 0x20, len(body), 0x41, 0, 0, "usb_hid_ota.bin")
    return outer + body


def main() -> int:
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        sdk = root / "sdk" / "cpu" / "wl82" / "tools"
        (sdk / "cfg").mkdir(parents=True)
        (sdk / "uboot.boot").write_bytes(bytes((i * 17 + 3) & 0xFF for i in range(14384)))
        (sdk / "cfg_tool.bin").write_bytes(bytes((i * 5 + 1) & 0xFF for i in range(383)))
        (sdk / "cfg" / "eq_cfg_hw.bin").write_bytes(bytes((i * 7 + 9) & 0xFF for i in range(2937)))

        app = fw.EXPECTED_ENTRY_STUB + b"SikkenWorks\0Born out of passion\0" + bytes(range(64))
        ota = ota_fixture()
        assert not fw.validate_app_binary(app)
        oi = fw.validate_ota(ota)
        assert oi["outer_header_ok"] and oi["inner_header_ok"] and oi["name_ok"] and oi["outer_data_crc_ok"]

        flash = fw.build_flash_image(app, root / "sdk")
        assert len(flash) == fw.FLASH_PACKAGE_BYTES
        pkg = fw.build_fwsc(flash, ota, "FM-1_015")
        out = root / "ETCH-0.23.fwsc.lab"
        out.write_bytes(pkg)
        report, errors = fw.verify_fwsc(out)
        assert not errors, errors
        assert report["product"] == "FM-1_015"
        assert report["chip"] == "AC791N"
        assert report["flash"]["chip_key"] == 0x980F
        assert report["flash"]["app_area"]["offset"] == fw.APP_XIP

        # V0.24: a user-supplied known-good package can be the exact source of
        # boot/config/OTA bytes, avoiding silent SDK/loader mismatch.
        assets = fw.extract_stock_assets(out)
        assert assets["key"] == fw.DEFAULT_CHIP_KEY
        assert assets["uboot"] == (sdk / "uboot.boot").read_bytes()
        assert len(assets["isd_config"]) >= 32
        assert assets["flash_header_payload"] == fw.FLASH_HEADER_PAYLOAD
        assert assets["cfg_tool"] == (sdk / "cfg_tool.bin").read_bytes()
        assert assets["eq_cfg"] == (sdk / "cfg" / "eq_cfg_hw.bin").read_bytes()
        assert assets["ota"] == ota
        rebuilt_flash = fw.build_flash_image_from_assets(
            app, assets["uboot"], assets["cfg_tool"], assets["eq_cfg"], assets["key"],
            isd_config=assets["isd_config"], flash_header_payload=assets["flash_header_payload"]
        )
        stock_repack = root / "stock-repack.fwsc.lab"
        stock_repack.write_bytes(fw.build_fwsc(rebuilt_flash, assets["ota"], "FM-1_015"))
        rr, re = fw.verify_fwsc(stock_repack)
        assert not re, re
        assert rr["product"] == "FM-1_015"

        # Stock-reference CLI reuses exact boot/config/OTA bytes while keeping ETCH's own package identity.
        app_path = root / "app.bin"
        app_path.write_bytes(app)
        cli_out = root / "from-stock.fwsc.lab"
        rc = fw.main(["lab-pack-from-stock", str(app_path), str(out), str(cli_out)])
        assert rc == 0
        cr, ce = fw.verify_fwsc(cli_out)
        assert not ce, ce
        assert cr["product"] == fw.DEFAULT_PRODUCT

        bad = bytearray(pkg)
        bad[-1] ^= 0x40
        badp = root / "bad.fwsc.lab"
        badp.write_bytes(bad)
        _, errors = fw.verify_fwsc(badp)
        assert errors, "corruption must be detected"

        proof = root / "R0_RECOVERY_PROVEN.json"
        proof.write_text(json.dumps({
            "status": "PROVEN",
            "hardware_revision": "TEST-REV",
            "stock_firmware": "FM-1 V15",
            "dump_1_sha256": "a" * 64,
            "dump_2_sha256": "a" * 64,
            "restore_1_stock_verified": True,
            "restore_2_stock_verified": True,
            "usb_midi_verified_after_restore": True,
            "audio_verified_after_restore": True,
            "controls_verified_after_restore": True,
        }))
        ok, errs = fw.validate_r0_proof(proof)
        assert ok and not errs

        audit = Path(str(app_path) + ".audit.json")
        audit.write_text(json.dumps({
            "valid": True,
            "app_sha256": fw.hashlib.sha256(app).hexdigest(),
            "entry_stub_ok": True,
            "expected_start": hex(fw.APP_XIP),
            "app_bytes": len(app),
            "errors": [],
            "notes": ["synthetic test fixture only"],
        }))
        audit_ok, audit_errs = fw.validate_app_audit(app_path, audit)
        assert audit_ok and not audit_errs

        # Full release command is exercised only with the synthetic test proof and
        # synthetic package fixture. It must require both proof + matching audit.
        release_out = root / "synthetic-release.fwsc"
        rc = fw.main(["release-pack-from-stock", str(app_path), str(out), str(release_out),
                      "--proof", str(proof), "--app-audit", str(audit)])
        assert rc == 0
        rel_report, rel_errors = fw.verify_fwsc(release_out)
        assert not rel_errors and rel_report["product"] == fw.DEFAULT_PRODUCT

        bad_audit_data = json.loads(audit.read_text())
        bad_audit_data["app_sha256"] = "0" * 64
        audit.write_text(json.dumps(bad_audit_data))
        audit_ok, audit_errs = fw.validate_app_audit(app_path, audit)
        assert not audit_ok and audit_errs

        proof_data = json.loads(proof.read_text()); proof_data["restore_2_stock_verified"] = False
        proof.write_text(json.dumps(proof_data))
        ok, errs = fw.validate_r0_proof(proof)
        assert not ok and errs
    print("fwsc tests: ok")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
