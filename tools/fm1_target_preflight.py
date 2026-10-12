#!/usr/bin/env python3
"""Read-only ETCH FM-1 target/release preflight. Never opens or flashes a device."""
from __future__ import annotations
import argparse, json, os, hashlib, sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
try:
    import fm1_fwsc
except Exception:
    fm1_fwsc = None


def sha256(path: Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()


def bridge_is_deliberately_verified(path: Path) -> bool:
    if not path.is_file():
        return False
    try:
        text=path.read_text(errors='replace')
    except Exception:
        return False
    return 'ETCH_FM1_BOARD_BRIDGE_VERIFIED_V1 1' in text and 'TODO' not in text


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--json',action='store_true')
    ap.add_argument('--allow-unproven-r0',action='store_true',help='report build readiness while keeping flash release blocked')
    ap.add_argument('--stock',type=Path,help='optional official/known-good FM-1 .fwsc for exact asset reuse')
    args=ap.parse_args()
    root=Path(__file__).resolve().parents[1]
    toolchain=Path(os.environ.get('JIELI_TOOLCHAIN',str(Path.home()/'.jieli/toolchain')))
    sdk=Path(os.environ.get('AC79_SDK',str(Path.home()/'fw-AC79_AIoT_SDK')))
    clang=toolchain/'pi32v2/bin/clang'; ld=toolchain/'pi32v2/bin/ld'
    objdump=toolchain/'common/bin/objdump'; objcopy=toolchain/'common/bin/objcopy'
    sdk_tool=sdk/'cpu/wl82/tools'
    bridge=root/'firmware/fm1-r2-test/fm1_sdk_bridge.cpp'
    proof=root/'R0_RECOVERY_PROVEN.json'
    packer=root/'tools/fm1_fwsc.py'
    app_validator=root/'tools/fm1_app_validate.py'
    elf_builder=root/'tools/fm1_elf_to_app.py'
    crt0=Path(os.environ['ETCH_FM1_CRT0']) if os.environ.get('ETCH_FM1_CRT0') else None
    linker=Path(os.environ['ETCH_FM1_LINKER_SCRIPT']) if os.environ.get('ETCH_FM1_LINKER_SCRIPT') else None
    checks={
        'jieli_pi32v2_clang': clang.is_file(),
        'jieli_pi32v2_ld': ld.is_file(),
        'jieli_objdump': objdump.is_file(),
        'jieli_objcopy': objcopy.is_file(),
        'sdk_uboot': (sdk_tool/'uboot.boot').is_file(),
        'sdk_cfg_tool': (sdk_tool/'cfg_tool.bin').is_file(),
        'sdk_eq_cfg': (sdk_tool/'cfg/eq_cfg_hw.bin').is_file(),
        'verified_board_bridge': bridge_is_deliberately_verified(bridge),
        'verified_crt0': bool(crt0 and crt0.is_file()),
        'verified_linker_script': bool(linker and linker.is_file()),
        'app_validator': app_validator.is_file(),
        'elf_to_app_builder': elf_builder.is_file(),
        'fwsc_packer': packer.is_file(),
        'r0_recovery_proof_marker': proof.is_file(),
    }
    proof_valid=False
    proof_errors=[]
    if checks['r0_recovery_proof_marker'] and fm1_fwsc:
        proof_valid, proof_errors=fm1_fwsc.validate_r0_proof(proof)
    checks['r0_recovery_proof_valid']=proof_valid
    source_ready=checks['jieli_pi32v2_clang'] and checks['verified_board_bridge']
    app_link_ready=source_ready and all(checks[k] for k in ('jieli_pi32v2_ld','jieli_objdump','jieli_objcopy','verified_crt0','verified_linker_script','app_validator','elf_to_app_builder'))
    package_deps=all(checks[k] for k in ('sdk_uboot','sdk_cfg_tool','sdk_eq_cfg','fwsc_packer'))
    stock_info=None
    stock_ready=False
    if args.stock:
        stock_info={'path':str(args.stock),'exists':args.stock.is_file(),'sha256':sha256(args.stock) if args.stock.is_file() else None}
        if args.stock.is_file() and fm1_fwsc:
            try:
                report, errors=fm1_fwsc.verify_fwsc(args.stock)
                stock_info['valid']=not errors; stock_info['errors']=errors; stock_info['product']=report.get('product')
                if not errors:
                    assets=fm1_fwsc.extract_stock_assets(args.stock)
                    stock_info['asset_hashes']=assets['hashes']; stock_ready=True
            except Exception as e:
                stock_info['valid']=False; stock_info['errors']=[str(e)]
    package_ready=checks['fwsc_packer'] and (package_deps or stock_ready)
    release_ready=app_link_ready and package_ready and proof_valid
    result={
        'target':'M-VAVE FM-1 / JieLi AC791N-WL82 / pi32v2',
        'toolchain':str(toolchain),'sdk':str(sdk),
        'checks':checks,
        'source_object_build_ready':source_ready,
        'app_link_ready':app_link_ready,
        'sdk_package_assets_ready':package_deps,
        'stock_reference_ready':stock_ready,
        'package_build_ready':package_ready,
        'flash_release_gate_ready':release_ready,
        'stock_reference':stock_info,
        'r0_proof_errors':proof_errors,
        'crt0':str(crt0) if crt0 else None,
        'linker_script':str(linker) if linker else None,
    }
    if args.json:
        print(json.dumps(result,indent=2))
    else:
        print('ETCH FM-1 target / app.bin / .fwsc preflight')
        for k,v in checks.items(): print(f"[{'OK' if v else '--'}] {k}")
        print('Target object build:', 'READY' if source_ready else 'BLOCKED')
        print('Audited app.bin link:', 'READY' if app_link_ready else 'BLOCKED')
        print('SDK package assets:', 'READY' if package_deps else 'BLOCKED')
        if args.stock: print('Exact stock asset reuse:', 'READY' if stock_ready else 'BLOCKED')
        print('.fwsc package stage:', 'READY' if package_ready else 'BLOCKED')
        print('Flashable release gate:', 'READY' if release_ready else 'BLOCKED')
        if not proof_valid:
            print('R0 proof is absent or invalid: audited app/lab package work is allowed; release .fwsc creation remains blocked.')
            for e in proof_errors: print('  R0:', e)
    hard=not source_ready or not package_ready or (not proof_valid and not args.allow_unproven_r0)
    return 2 if hard else 0

if __name__=='__main__': raise SystemExit(main())
