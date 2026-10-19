#!/usr/bin/env python3
"""Replays two release packages against the exact archived V1.38 C OTA code.
Runs with a RAM-backed simulated NOR; never accesses a physical FM1.
"""
from pathlib import Path
import subprocess,tempfile,os,re,hashlib
R=Path(__file__).resolve().parents[1]
head=R/'tests/fixtures/v138_stock_head.bin'
assert hashlib.sha256(head.read_bytes()).hexdigest()=='d9f43191777656de01c7bed4f9e9ba2e34e94832c5d2ed02c3fc7ab7a6d9bd67'
with tempfile.TemporaryDirectory() as td:
 exe=Path(td)/'legacy_v138_check'
 cc=os.environ.get('CC','cc')
 subprocess.run([cc,'-O2','-std=c11','-I',str(R/'firmware/fm1-native'),'-o',str(exe),str(R/'tests/test_v1482_v138_ota.c'),str(R/'firmware/fm1-native/native_digest.c')],check=True)
 def run(variant):
  pkg=R/f'release/ETCH-FM1-V1.50-{variant}-HEAD.fwsc'
  p=subprocess.run([str(exe),str(pkg),str(head)],text=True,capture_output=True,timeout=30)
  line=next((s for s in p.stdout.splitlines() if s.startswith('CROSS_OTA_RESULT=')),None)
  assert line is not None,(p.stdout,p.stderr)
  vals=dict((k,int(v)) for k,v in re.findall(r'(\w+)=(-?\d+)',line))
  return p.returncode,vals
 badrc,bad=run('ETCH')
 goodrc,good=run('STOCK')
 assert (badrc,bad['CROSS_OTA_RESULT'],bad['REQUESTS'],bad['COMMITTED'])==(1,-5,18,0),(badrc,bad)
 assert (goodrc,good['CROSS_OTA_RESULT'],good['REQUESTS'],good['COMMITTED'],good['BOOT_HEAD_UNCHANGED'])==(0,0,65,1,1),(goodrc,good)
 print('PASS preserved V1.38 C OTA: ETCH head rejected -5 after 18 reads; V15 head accepted after 65; no protected-head writes')
