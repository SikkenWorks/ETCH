#!/usr/bin/env python3
"""Export an exact, byte-identical .fwsc from an ETCH webinstaller.

This is a *format and consistency* gate, not a hardware-safety or recovery
certification. Required precondition for public distribution: the device owner
has reported successful physical installation of a compatible preceding ETCH
firmware. This program never communicates with or flashes the FM1.
"""
import argparse,base64,hashlib,re,sys
from pathlib import Path
import fm1_fwsc as fw

def extract(installer:Path, output:Path, expected_id:str, reference:Path):
    if output.suffix.lower()!='.fwsc':raise ValueError('Expected a real .fwsc output path')
    html=installer.read_text(encoding='utf-8')
    payloads=re.findall(r'<script id="firmware-embedded" type="application/octet-stream">\s*([\s\S]*?)\s*</script>',html)
    if len(payloads)!=1:raise ValueError('Installer must embed exactly one firmware payload')
    data=base64.b64decode(''.join(payloads[0].split()),validate=True)
    if f"ID_ET: '{expected_id}'" not in html:raise ValueError('Installer identity mismatch')
    wanted=re.search(r"HASH_ET: '([0-9a-f]{64})'",html)
    if not wanted or hashlib.sha256(data).hexdigest()!=wanted.group(1):raise ValueError('Embedded firmware checksum mismatch')
    if re.search(r'ET_LENGTH: (\d+)',html) and int(re.search(r'ET_LENGTH: (\d+)',html).group(1))!=len(data):raise ValueError('Embedded firmware length mismatch')
    # Temporarily verify the embedded container without creating another
    # public firmware file; read-only verifier does no device I/O.
    from tempfile import TemporaryDirectory
    with TemporaryDirectory() as td:
        p=Path(td)/'candidate.fwsc'
        p.write_bytes(data)
        report,errors=fw.verify_fwsc(p)
        if errors or report['product']!=expected_id:raise ValueError('Firmware invalid: '+repr(errors)+' '+str(report['product']))
    r,errors=fw.verify_fwsc(reference)
    if errors:raise ValueError('Reference invalid: '+str(errors))
    def payload(raw,name):
        p=fw.parse_ufw(raw)
        return next(x['data'] for x in p['entries'] if x['name']==name)
    if payload(data,'flash.bin')[:0x4000]!=payload(reference.read_bytes(),'flash.bin')[:0x4000]:
        raise ValueError('Protected 16KiB boot header differs from previous tested candidate')
    if payload(data,'ota.bin')!=payload(reference.read_bytes(),'ota.bin'):
        raise ValueError('OTA loader differs from previous tested candidate')
    output.write_bytes(data)
    if output.read_bytes()!=data:raise IOError('Output verification failed')
    print(f'PASS real .fwsc container {output.name}; {len(data)} bytes; id={expected_id}; SHA256={hashlib.sha256(data).hexdigest()}')
    return data

if __name__=='__main__':
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('installer',type=Path);a.add_argument('output',type=Path)
    a.add_argument('--identity',required=True);a.add_argument('--reference',type=Path,required=True)
    args=a.parse_args()
    extract(args.installer,args.output,args.identity,args.reference)
