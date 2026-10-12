import sys,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from fm1_lz4 import decode_block,decode_loader
from fm1_fwsc import validate_ota,jlf_entry,crc16
raw=b'ETCH';block=b'\x40'+raw
assert decode_block(block,4)==raw
encoded=struct.pack('<II',len(block),len(raw))+block
inner=jlf_entry(crc16(raw),len(raw),0x01c0a800,0,0,0,'usb_hid_ota.bin');body=inner+encoded
outer=jlf_entry(crc16(body),32,len(body),0x41,0,0,'usb_hid_ota.bin')
assert validate_ota(outer+body)['inner_data_crc_ok']
for bad in [b'\xf0',b'\x00\x00\x00',b'\x40ET',b'\x10A\x01\x00']:
 try:decode_block(bad,4)
 except ValueError:pass
 else:raise AssertionError('malformed block accepted')
try:decode_loader(encoded+b'x',4)
except ValueError:pass
else:raise AssertionError('trailing bytes accepted')
print('LZ4 loader bounds, CRC and malformed-stream tests passed')
