#!/usr/bin/env python3
"""Bounded, parallel, isolated native regression runner for ETCH V0.45."""
import ast,os,subprocess,tempfile,sys
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor,as_completed
root=Path(__file__).resolve().parents[1];native=root/'firmware/fm1-native'
module=ast.parse((root/'tools/run_native_tests.py').read_text())
cases={}
for stmt in module.body:
 if isinstance(stmt,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='cases' for t in stmt.targets):cases=ast.literal_eval(stmt.value)
assert len(cases)>=35
results={}
with tempfile.TemporaryDirectory(prefix='etch-v045-regression-') as work:
 def run(t):
  name,sources=t;target=Path(work)/name
  args=['gcc','-std=c11','-O1','-g','-fsanitize=address,undefined','-ffunction-sections','-Wl,--gc-sections',str(root/f'tests/test_native_{name}.c'),*[str(native/x) for x in sources],'-o',str(target)]
  built=subprocess.run(args,capture_output=True,text=True,timeout=35)
  if built.returncode:return 'FAIL COMPILE '+built.stderr[-900:]
  tested=subprocess.run([str(target)],cwd=root,capture_output=True,text=True,timeout=60,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
  return ('PASS '+tested.stdout[-160:].strip()) if tested.returncode==0 else ('FAIL RUN '+tested.stderr[-900:])
 with ThreadPoolExecutor(max_workers=3) as pool:
  fut={pool.submit(run,t):t[0] for t in cases.items()}
  for item in as_completed(fut):
   name=fut[item]
   try:result=item.result()
   except Exception as e:result='FAIL '+repr(e)
   results[name]=result
   print(name,result,flush=True)
print('RESULT',sum(x.startswith('PASS') for x in results.values()),'PASS',sum(not x.startswith('PASS') for x in results.values()),'FAIL',flush=True)
if any(not x.startswith('PASS') for x in results.values()):sys.exit(1)
