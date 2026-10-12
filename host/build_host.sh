#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${ETCH_HOST_OUT:-$ROOT/build/host}"
CC="${CC:-cc}"
mkdir -p "$OUT"
"$CC" -std=c11 -O1 -g -Wall -Wextra -Wno-misleading-indentation -Wno-sign-compare -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I"$ROOT/firmware/fm1-native" \
  "$ROOT/host/etch_host.c" "$ROOT/firmware/fm1-native/audio_core.c" \
  "$ROOT/firmware/fm1-native/native_performance.c" "$ROOT/firmware/fm1-native/native_wave_fx.c" "$ROOT/firmware/fm1-native/native_fx.c" "$ROOT/firmware/fm1-native/wave_editor.c" \
  -o "$OUT/etch_host"
echo "Built $OUT/etch_host (native FM1 audio/DSP host executable)."
