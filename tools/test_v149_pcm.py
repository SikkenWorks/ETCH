#!/usr/bin/env python3
"""Release-blocking sample-level equivalence: V1.48.5 vs V1.49."""
from pathlib import Path
import subprocess,tempfile,re,hashlib,sys
R=Path(__file__).resolve().parents[1]
N=R/'firmware/fm1-native'
F=R/'tests/test_v1462_realtime_pcm.c'
BASE=R/'tests/reference/V1.48.5-audio_core.c'
FX=R/'tests/reference/V1.45-native_fx.c'
if (N/'native_fx.c').read_bytes()!=FX.read_bytes():
 sys.exit('BLOCK: previously crashing V1.46 FX changes or other unverified FX modification detected')
if BASE.read_bytes()==(N/'audio_core.c').read_bytes():
 sys.exit('BLOCK: candidate audio core matches baseline, optimization missing')
with tempfile.TemporaryDirectory(prefix='etch149_pcm_') as tmp:
 checks=[]
 for title,core in [('V1.48.5 baseline',BASE),('V1.49 candidate',N/'audio_core.c')]:
  exe=Path(tmp)/('reference' if title.startswith('V1.48.5') else 'candidate')
  cmd=['gcc','-O2','-std=c11','-I',str(N),str(F),str(core),str(N/'native_performance.c'),str(N/'native_fx.c'),'-o',str(exe)]
  subprocess.run(cmd,check=True,capture_output=True,timeout=40)
  run=subprocess.run([str(exe)],capture_output=True,text=True,check=True,timeout=60)
  matches=re.findall(r'scenario=(\d+) pcm=([0-9a-f]{16})',run.stdout)
  if len(matches)!=7:sys.exit(f'BLOCK: {title} missing PCM checksums')
  checks.append(matches)
 if checks[0]!=checks[1]:sys.exit(f'BLOCK: changed PCM output: {checks}')
 print('PASS: 7/7 bit-exact PCM scenarios vs V1.48.5, including GROUPS/FX')
 print('BASELINE_SOURCE_SHA256:',hashlib.sha256(BASE.read_bytes()).hexdigest())
 print('NOTE: bit parity on host != proven FM1 CPU deadline or physical install')
