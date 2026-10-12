"""Bounded raw LZ4-block decoder for FM1 loader inspection."""
import struct

def decode_block(data: bytes, expected: int) -> bytes:
 if not 0<expected<=4096:raise ValueError('invalid LZ4 output length')
 out=bytearray();i=0
 def length(base):
  nonlocal i
  if base==15:
   while True:
    if i>=len(data):raise ValueError('truncated LZ4 length')
    x=data[i];i+=1;base+=x
    if x!=255:break
  return base
 while i<len(data):
  token=data[i];i+=1;n=length(token>>4)
  if i+n>len(data) or len(out)+n>expected:raise ValueError('LZ4 literal bounds')
  out.extend(data[i:i+n]);i+=n
  if i==len(data):break
  if i+2>len(data):raise ValueError('truncated LZ4 match')
  distance=data[i]|data[i+1]<<8;i+=2
  if distance==0 or distance>len(out):raise ValueError('invalid LZ4 distance')
  n=length(token&15)+4
  if len(out)+n>expected:raise ValueError('LZ4 match bounds')
  for _ in range(n):out.append(out[-distance])
 if len(out)!=expected:raise ValueError('LZ4 decoded length mismatch')
 return bytes(out)

def decode_loader(data: bytes, expected: int) -> bytes:
 if not 0<expected<=0x14000:raise ValueError('loader output exceeds bound')
 out=bytearray();i=0
 while i<len(data):
  if i+8>len(data):raise ValueError('truncated loader block header')
  clen,dlen=struct.unpack_from('<II',data,i);i+=8
  if not 0<clen<=4096 or i+clen>len(data) or len(out)+dlen>expected:raise ValueError('loader block bounds')
  out.extend(decode_block(data[i:i+clen],dlen));i+=clen
 if len(out)!=expected:raise ValueError('loader size mismatch')
 return bytes(out)
