## sharkpsx (dev) Documentation

## Building
```
mkdir build && cd build
cmake ..
make
```
Resulting executable will be in `build/src/main`.

## Builtin Debugger
### Operation
The main executable is the builtin debugger, which is the main way for interacting with the emulator (including for non-debugging purposes). You can issue simple commands in the form `command arg1 arg2 ...`. Note command names may be multiple words long. Some aliases are defined to vaguely reflect GDB.

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
sys watchpoint set read addr:HEX          Set a read watchpoint for bus address addr
sys watchpoint set write addr:HEX         Set a write watchpoint for bus address addr
sys watchpoint remove addr:HEX            Remove all the watchpoints at addr
sys sideload set path:STR                 Cause sideloading of PS EXE at path when reaching shell
sys sideload remove                       Remove the set sideload
quit                                      Exit
```
