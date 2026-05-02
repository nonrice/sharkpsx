## sharkpsx (dev) Documentation
## Design Overview
sharkpsx aims to achieve high level emulation (HLE) of the Playstation 1 ("PSX"), and thus is not overly concerned with reflecting the true [architecture](https://www.copetti.org/writings/consoles/playstation/) of the PSX. Instead, the overall architecture is nicely abstracted by a single bus model, as memory-mapped I/O forms the backbone of the PSX's design.

## Building
```
mkdir build && cd build
cmake ..
make
```

## Debugger
### Argument Types
```
HEX   Unsigned 32-bit hex value, with or without 0x prefix
DEC   Unsigned 32-bit decimal value
STR   String, with or without surrounding double quotes ("")
```
### Commands
Arguments are whitespace separated, and are all required. Parentheses `()` reflect possible aliases for the keyword.
```
cpu dump                                  Dump all CPU registers
cpu setpc addr:HEX                        Set current program counter to addr
mem examine(x) addr:HEX num:DEC           Print num 32-bit words starting from addr
mem writefile addr:HEX path:STR           Write the contents of path into memory starting at addr
mem disassemble(disas) addr:HEX num:DEC   Disassemble num instruction starting from addr
bios writefile path:STR                   Write contents of path into the BIOS ROM
sys run                                   Start execution
sys breakpoint(br) set addr:HEX           Add a breakpoint at addr
sys breakpoint(br) remove addr:HEX        Remove breakpoint at addr
sys breakpoint(br) list                   Show all breakpoints
```

## Development References
- https://www.cs.cmu.edu/afs/cs/academic/class/15213-s26/www/
- https://psx-spx.consoledev.net
- https://jsgroth.dev/blog/posts/ps1-sideloading/
- https://www.psx.dev/
- https://github.com/grumpycoders/pcsx-redux
- http://www.cs.iit.edu/~virgil/cs470/Labs/Lab7.pdf
- https://ftp.zx.net.nz/pub/micro/wumips/79R3041/ur\_manual/843.pdf
