## sharkpsx (dev) Documentation
## Design Overview
sharkpsx aims to achieve high level emulation (HLE) of the Playstation 1 ("PSX"). Due to memory-mapped IO the overall architecture is nicely abstracted by a Bus model where the CPU communicates with a single bus that maps operations to various devices.

## Building
```
mkdir build && cd build
cmake ..
make
```
## Debugger
### Operation
The main executable is the debugger, which is the main way for interacting with the emulator (including for non-debugging purposes). You can issue simple commands in the form `command arg1 arg2 ...`. Note command names may be multiple words long. Some aliases are defined to vaguely reflect GDB.

By supplying a filepath as a command line argument, the debugger reads commands from the file. Note that if the file doesn't contain the `quit` command the debugger will just continue to the command line prompt.
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
sys watchpoint set read addr:HEX          Set a read watchpoint for bus address addr
sys watchpoint set write addr:HEX         Set a write watchpoint for bus address addr
sys watchpoint list                       List current watchpoints
sys watchpoint remove addr:HEX            Remove all the watchpoints at addr
hex2dec(h2d) h:HEX                        Convert unsigned hex to decimal
dec2hex(d2h) d:DEC                        Convert unsigned decimal to hex
quit                                      Exit
```

## Development References
- https://www.cs.cmu.edu/afs/cs/academic/class/15213-s26/www/
- https://psx-spx.consoledev.net
- https://jsgroth.dev/blog/posts/ps1-sideloading/
- https://www.psx.dev/
- https://github.com/grumpycoders/pcsx-redux
- http://www.cs.iit.edu/~virgil/cs470/Labs/Lab7.pdf
- https://ftp.zx.net.nz/pub/micro/wumips/79R3041/ur_manual/843.pdf
