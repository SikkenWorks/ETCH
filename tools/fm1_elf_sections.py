"""Read ELF32 little-endian section VMA/LMA directly, without objdump dialect assumptions."""
import struct
from pathlib import Path

def elf_sections(path: Path) -> list[dict]:
 b=path.read_bytes()
 if b[:6]!=b'\x7fELF\x01\x01':raise ValueError('expected little-endian ELF32')
 h=struct.unpack_from('<16sHHIIIIIHHHHHH',b)
 phoff,shoff=h[5],h[6];phsize,phnum,shsize,shnum,stridx=h[9:14]
 if phsize<32 or shsize<40 or stridx>=shnum:raise ValueError('invalid ELF header')
 ph=[struct.unpack_from('<IIIIIIII',b,phoff+i*phsize) for i in range(phnum)]
 sh=[struct.unpack_from('<IIIIIIIIII',b,shoff+i*shsize) for i in range(shnum)]
 strings=b[sh[stridx][4]:sh[stridx][4]+sh[stridx][5]]
 out=[]
 for s in sh:
  nameoff,kind,flags,vma,off,size=s[:6]
  if not flags&2:continue
  end=strings.find(b'\0',nameoff)
  if end<0:raise ValueError('invalid section name')
  name=strings[nameoff:end].decode('ascii');lma=vma
  if kind!=8 and size:
   matches=[p for p in ph if p[0]==1 and p[1]<=off and off+size<=p[1]+p[4]]
   if len(matches)!=1:raise ValueError('ambiguous load mapping for '+name)
   lma=matches[0][3]+off-matches[0][1]
   if off+size>len(b):raise ValueError('truncated section '+name)
  out.append(dict(name=name,size=size,vma=vma,lma=lma,file_offset=off,nobits=kind==8))
 return out
