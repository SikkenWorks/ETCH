#!/usr/bin/env python3
"""Independent bounded native test runner, no shared long-lived test process."""
from pathlib import Path
import subprocess,ast,os,sys,tempfile
r=Path(__file__).resolve().parents[1];n=r/'firmware/fm1-native'
source=(r/'tools/run_native_tests.py').read_text()
module=ast.parse(source)
cases=None
for item in module.body:
 if isinstance(item,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='cases' for t in item.targets):cases=ast.literal_eval(item.value)
assert cases
fails=[];passes=[]
with tempfile.TemporaryDirectory(prefix='etch-v044-tests-') as tmp:
 for name,src in cases.items():
  out=Path(tmp)/name
  cmd=['gcc','-std=c11','-O1','-g','-fsanitize=address,undefined','-ffunction-sections','-Wl,--gc-sections',str(r/f'tests/test_native_{name}.c'),*[str(n/f) for f in src],'-o',str(out)]
  try:
   b=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,timeout=20)
   if b.returncode:raise RuntimeError('COMPILE '+b.stderr[-500:])
   t=subprocess.run([str(out)],cwd=r,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,timeout=14)
   if t.returncode:raise RuntimeError('RUN '+t.stderr[-500:])
   print('PASS',name,flush=True);passes.append(name)
  except Exception as e:print('FAIL',name,str(e),flush=True);fails.append(name)
 print('RESULT',len(passes),'PASS',len(fails),'FAIL',','.join(fails),flush=True)
 if fails:sys.exit(1)
