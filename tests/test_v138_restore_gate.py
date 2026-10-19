#!/usr/bin/env python3
"""Fail CI if OTA-to-stock compatibility is accidentally removed from ETCH."""
from pathlib import Path
import re,os,hashlib,sys
ROOT=Path(__file__).resolve().parents[1]
src=(ROOT/'firmware/fm1-native/ota.c').read_text()
assert '#define ETCH_STOCK_OTA_BODY_LEN 19937u' in src
assert 'etch_ota_stock(fl_off,b)' in src
assert 'etch_ota_hash_pinned(ota_off+32,b,1,len,pinned_digest)' in src
assert 'etch_ota_hash_pinned(OTA_AREA+32,b,0,len,pinned_digest)' in src
assert 'etch_host_app_compatible(fl_off,b)' in src, 'Reject unknown application memory layouts before staging.'
assert 'etch_trusted_app_loader' in src, 'Never stage the foreign executable loader.'
assert 'etch_host_loader_crc(' in src, 'Reject corrupted external app-only loader packages.'
assert 'if(!etch_ota_available(b))return -15;' in src
assert (ROOT/'firmware/fm1-native/bringup.c').read_text().count('#define FELUCCA_ID "FM-1_1500"')==1
ref=os.environ.get('FM1_STOCK_V15')
if ref:
 sys.path.insert(0,str(ROOT/'tools'))
 from fm1_fwsc import parse_ufw
 raw=Path(ref).read_bytes()
 assert hashlib.sha256(raw).hexdigest()=='db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a'
 pkg=parse_ufw(raw)
 flash=next(e['data'] for e in pkg['entries'] if e['type']==0)
 loader=next(e['data'] for e in pkg['entries'] if e['type']==100)
 assert hashlib.sha256(flash).hexdigest()=='c1068a403c04a6378a626afb66cc217f72d13f10aa200e983b7a9817cc1dac03'
 assert len(loader)==19969
 assert hashlib.sha256(loader[32:]).hexdigest()=='a19bb1bee72c12af9db218fd713b903273ad9d4ae868d1300d8041d6235dc134'
 print('PASS V1.50: preserved stock V15 exact reference and full restore gates')
else:
 print('PASS V1.50: source policy guards (set FM1_STOCK_V15 for full exact-stock verification)')
