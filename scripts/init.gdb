set architecture mips:3000
set endian little

mem 0x80000000 0x8007ffff rw
mem 0xbfc00000 0xbfc7ffff ro

set remotetimeout 5

source scripts/gte_disas.py
