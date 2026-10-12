#!/usr/bin/env bash
# Native FM1 audio-core host scenarios, no USB/device access.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT/host/build_host.sh"
OUT="$ROOT/build/host/scenarios"
mkdir -p "$OUT"
for scenario in "$ROOT"/host/scenarios/*.etch; do
  base="$(basename "$scenario" .etch)"
  "$ROOT/build/host/etch_host" "$scenario" "$OUT/$base.wav" "$OUT/$base.json"
done
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys, wave, json
root=Path(sys.argv[1]); results=[]
for meta in sorted(p for p in root.glob('*.json') if p.stem != 'summary'):
    report=json.loads(meta.read_text()); filename=meta.with_suffix('.wav')
    with wave.open(str(filename),'rb') as wf:
        assert wf.getnchannels()==1 and wf.getsampwidth()==2 and wf.getframerate()==44100
        assert wf.getnframes()==report['frames']
    assert report['assertions_passed']>0 and report['peak_absolute']>0
    results.append({'scenario':meta.stem,**report})
assert len(results)==7, 'expected exactly seven native host scenarios'
(root/'summary.json').write_text(json.dumps({'result':'PASS','scope':'native audio/FX/wave editor/performance ONLY; no hardware HAL/UI/USB test','scenarios':results},indent=2)+'\n')
print(f'PASS: {len(results)} verified native FM1-host scenarios; WAV headers and PCM statistics validated')
PY
