#!/usr/bin/env python3
"""Read-only ETCH/Felucca P04 flash dump inspection. NEVER writes device/flash.

Usage (Windows): py tools/p04_verify_flash_dump.py C:\\path\\to\\fm1-full-flash.bin
Exact 1 MiB original dump required. A BLANK result is not proof of a safe
physical partition or boot/recovery, merely evidence of currently erased bytes.
"""
import argparse, hashlib
from pathlib import Path

SIZE=0x100000
REGIONS={
    'ETCH legacy A/B':(0x97000,0xA1000),
    'ETCH proposed expanded A':(0xA1000,0xB1000),
    'ETCH proposed expanded B':(0xB1000,0xC1000),
    'Felucca sample slots':(0xA0000,0xDC000),
    'Felucca OTA staging':(0xE0000,0xE5000),
}

def inspect(data:bytes):
    if len(data)!=SIZE:raise ValueError(f'Expected exactly {SIZE:,} bytes, got {len(data):,}')
    print('Full flash dump SHA-256:',hashlib.sha256(data).hexdigest())
    for label,(start,end) in REGIONS.items():
        area=data[start:end];used=sum(b!=255 for b in area)
        print(f'{label:28s} 0x{start:05X}..0x{end:05X}  {used:7,d} non-FF bytes',
              '(BLANK)' if not used else '(OCCUPIED)')
    marker=data[0xA0000:0xA0004]
    if marker==b'FSMP':print('Felucca sample slot 0: FSMP signature PRESENT (collision: do not claim expanded area)')
    elif marker==b'\xff'*4:print('Felucca sample slot 0: erased marker')
    else:print('Felucca sample slot 0: unrecognized marker',marker.hex())
    extended=data[0xA1000:0xC1000]
    blank=all(x==255 for x in extended)
    print('Storage candidate:', 'PHYSICALLY BLANK; STILL UNVERIFIED FOR WRITES' if blank and marker!=b'FSMP'
          else 'BLOCKED / OCCUPIED: DO NOT FLASH OR ERASE THIS REGION')
    print('IMPORTANT: byte-level inspection cannot establish safe partition ownership,')
    print('ROM recovery, updater preservation, or compatibility with another firmware.')
    return 0 if blank and marker!=b'FSMP' else 2

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('dump',type=Path,help='Previously verified 1 MiB FM1 full flash dump')
    args=p.parse_args()
    try:raise SystemExit(inspect(args.dump.read_bytes()))
    except (OSError,ValueError) as e:p.error(str(e))
