#!/usr/bin/env python3
"""READ ONLY: compare two user-supplied complete dumps. Never opens a device."""
import argparse, hashlib, json
from pathlib import Path

def inspect(path):
    digest=hashlib.sha256();size=0
    with path.open('rb') as f:
        while chunk:=f.read(1024*1024):
            size+=len(chunk);digest.update(chunk)
    return {'file':str(path),'bytes':size,'sha256':digest.hexdigest()}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('first',type=Path);parser.add_argument('second',type=Path)
    args=parser.parse_args()
    try: a,b=inspect(args.first),inspect(args.second)
    except OSError as error: parser.exit(2,str(error)+'\n')
    same=a['bytes']>0 and a['bytes']==b['bytes'] and a['sha256']==b['sha256']
    print(json.dumps({'first':a,'second':b,'matching_nonempty_files':same,
        'recovery_proven':False,'note':'Matching files do not prove dump completeness, device identity or a working restore.'},indent=2))
    return 0 if same else 1
if __name__=='__main__': raise SystemExit(main())
