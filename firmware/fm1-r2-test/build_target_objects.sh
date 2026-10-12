#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
CLANG="${JIELI_CLANG:-$TOOLCHAIN/pi32v2/bin/clang}"
OUT="$ROOT/build/fm1-r2-test"
BRIDGE="$ROOT/firmware/fm1-r2-test/fm1_sdk_bridge.cpp"
fail(){ echo "ETCH FM-1 object build: $*" >&2; exit 2; }
mkdir -p "$OUT"
[ -x "$CLANG" ] || fail "missing JieLi pi32v2 clang: $CLANG"
[ -f "$BRIDGE" ] || fail "missing verified fm1_sdk_bridge.cpp"
if ! grep -q 'ETCH_FM1_BOARD_BRIDGE_VERIFIED_V1 1' "$BRIDGE" || grep -q 'TODO' "$BRIDGE"; then
  fail "fm1_sdk_bridge.cpp is not marked verified or still contains TODOs"
fi
COMMON=( -target pi32v2 -integrated-as -Oz -ffreestanding -fno-exceptions -fno-rtti -std=c++17 \
  -I"$ROOT/engine" -I"$ROOT/target" -I"$ROOT/hal" -I"$ROOT/firmware/fm1-r2-test" )
for src in \
 "$ROOT/engine/boot_state.cpp" "$ROOT/engine/watchdog.cpp" "$ROOT/target/r2_app.cpp" \
 "$ROOT/target/freestanding_runtime.cpp" \
 "$ROOT/firmware/fm1-r2-test/fm1_platform_port.cpp" \
 "$ROOT/firmware/fm1-r2-test/etch_fm1_test_main.cpp" \
 "$BRIDGE"; do
  obj="$OUT/$(basename "${src%.cpp}").o"
  "$CLANG" "${COMMON[@]}" -c "$src" -o "$obj"
done
printf 'Target objects built in %s\n' "$OUT"
printf 'NO app.bin/.fwsc WAS PRODUCED. Linker/startup/package integration remains gated.\n'
