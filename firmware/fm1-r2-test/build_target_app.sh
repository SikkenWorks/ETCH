#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
CLANG="${JIELI_CLANG:-$TOOLCHAIN/pi32v2/bin/clang}"
LD="${JIELI_LD:-$TOOLCHAIN/pi32v2/bin/ld}"
CRT0="${ETCH_FM1_CRT0:-}"
LINKER="${ETCH_FM1_LINKER_SCRIPT:-}"
OUT="$ROOT/build/fm1-r2-test"
BRIDGE="$ROOT/firmware/fm1-r2-test/fm1_sdk_bridge.cpp"
mkdir -p "$OUT"

fail(){ echo "ETCH FM-1 app build: $*" >&2; exit 2; }
[ -x "$CLANG" ] || fail "missing JieLi pi32v2 clang: $CLANG"
[ -x "$LD" ] || fail "missing JieLi pi32v2 ld: $LD"
[ -f "$BRIDGE" ] || fail "missing verified fm1_sdk_bridge.cpp (do not rename the template without verified board mappings)"
if ! grep -q 'ETCH_FM1_BOARD_BRIDGE_VERIFIED_V1 1' "$BRIDGE" || grep -q 'TODO' "$BRIDGE"; then
  fail "fm1_sdk_bridge.cpp is not marked verified or still contains TODOs"
fi
[ -n "$CRT0" ] && [ -f "$CRT0" ] || fail "set ETCH_FM1_CRT0 to a verified AC791N/WL82 startup source/object"
[ -n "$LINKER" ] && [ -f "$LINKER" ] || fail "set ETCH_FM1_LINKER_SCRIPT to a verified FM-1 linker script"

COMMON=( -target pi32v2 -integrated-as -Oz -ffreestanding -ffunction-sections -fdata-sections \
  -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit -std=c++17 \
  -I"$ROOT/engine" -I"$ROOT/target" -I"$ROOT/hal" -I"$ROOT/firmware/fm1-r2-test" )
SOURCES=(
 "$ROOT/engine/boot_state.cpp"
 "$ROOT/engine/watchdog.cpp"
 "$ROOT/target/r2_app.cpp"
 "$ROOT/target/freestanding_runtime.cpp"
 "$ROOT/firmware/fm1-r2-test/fm1_platform_port.cpp"
 "$ROOT/firmware/fm1-r2-test/etch_fm1_test_main.cpp"
 "$BRIDGE"
)
OBJS=()
for src in "${SOURCES[@]}"; do
 obj="$OUT/$(basename "${src%.cpp}").o"
 "$CLANG" "${COMMON[@]}" -c "$src" -o "$obj"
 OBJS+=("$obj")
done
if [[ "$CRT0" == *.o ]]; then
 CRT_OBJ="$CRT0"
else
 CRT_OBJ="$OUT/etch_crt0.o"
 "$CLANG" -target pi32v2 -integrated-as -c "$CRT0" -o "$CRT_OBJ"
fi
ELF="$OUT/etch-fm1-r2.elf"
"$LD" --gc-sections -e _start -T "$LINKER" "$CRT_OBJ" "${OBJS[@]}" -o "$ELF"
"$TOOLCHAIN/common/bin/objdump" -t "$ELF" > "$OUT/etch-fm1-r2.syms.txt"
"$TOOLCHAIN/common/bin/objdump" -h "$ELF" > "$OUT/etch-fm1-r2.sections.txt"
python3 "$ROOT/tools/fm1_elf_to_app.py" "$ELF" "$OUT/app.bin" --toolchain "$TOOLCHAIN"
python3 "$ROOT/tools/fm1_app_validate.py" "$OUT/app.bin" --elf "$ELF" --toolchain "$TOOLCHAIN" --report "$OUT/app.bin.audit.json"
echo "Built audited target app: $OUT/app.bin"
echo "No .fwsc was produced. Use lab-pack-from-stock until the R0 release gate is proven."
