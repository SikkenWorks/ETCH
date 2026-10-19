#!/usr/bin/env python3
"""Golden PCM vs the unchanged V1.46.1 core, seven native scenarios.
No browser/FW emulation and no hardware-level speed claim.
"""
from pathlib import Path
import tempfile, subprocess, re, statistics, os, sys, hashlib
root=Path(__file__).resolve().parents[1]
native=root/'firmware/fm1-native'
fixture=root/'tests/reference/V1.46.1-audio_core.c'
golden=root/'tests/test_v1462_realtime_pcm.c'
if not fixture.exists():raise SystemExit('Missing reference audio_core source')
if fixture.read_bytes()==(native/'audio_core.c').read_bytes():raise SystemExit('Optimization source unchanged; wrong project?')
if (root/'tests/reference/V1.45-native_fx.c').read_bytes()!=(native/'native_fx.c').read_bytes():
 raise SystemExit('RELEASE BLOCK: FX differs from known V1.45 safe base')
outputs={}
with tempfile.TemporaryDirectory(prefix='etch1462-golden-') as t:
 for name,core in [('reference',fixture),('candidate',native/'audio_core.c')]:
  executable=Path(t)/name
  command=['gcc','-std=c11','-O2','-I',str(native),str(golden),str(core),
           str(native/'native_performance.c'),str(native/'native_fx.c'),'-o',str(executable)]
  subprocess.run(command,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,timeout=40)
  rows=[];elapsed=[]
  for n in range(5):
   out=subprocess.run([str(executable)],capture_output=True,text=True,check=True,timeout=20).stdout
   pairs=re.findall(r'scenario=(\d+) pcm=([0-9a-f]{16}) host_ns=(\d+)',out)
   if len(pairs)!=7:raise SystemExit(name+' failed to produce seven hashes')
   hashes=[x[1] for x in pairs]
   if rows and hashes!=rows[0]:raise SystemExit(name+' nondeterministic PCM')
   rows.append(hashes);elapsed.append([int(x[2]) for x in pairs])
  outputs[name]=(rows[0],[statistics.median(x[i] for x in elapsed) for i in range(7)])
a,at=outputs['reference'];b,bt=outputs['candidate']
if a!=b:
 for i,(x,y) in enumerate(zip(a,b)):
  if x!=y:print('MISMATCH',i,x,y,file=sys.stderr)
 raise SystemExit('RELEASE BLOCK: PCM differs from recovery baseline')
if len(set(a))!=7:raise SystemExit('RELEASE BLOCK: audio fixtures do not cover seven distinct outputs')
print('PASS 7/7 PCM bit exact against V1.46.1, with three GROUPS and global FX')
print('REFERENCE_SHA256',hashlib.sha256(fixture.read_bytes()).hexdigest())
for i in range(7):
 print(f'SCENARIO {i}: {a[i]} baseline_median_ns={int(at[i])} optimized_median_ns={int(bt[i])} host_relative={(bt[i]/at[i]):.3f}')
print('HOST NOTE: monotonic x86 timings do not validate 60 MHz emulator or physical FM1 audio deadline')
