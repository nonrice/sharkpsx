import gdb
import gdb.disassembler
from enum import Enum

class GTEDisasm(gdb.disassembler.Disassembler):
    gte_op_names = {
        0x01: "rtps",
        0x06: "nclip",  
        0x0c: "op",
        0x10: "dpcs",   
        0x11: "intpl",  
        0x12: "mvmva",  
        0x13: "ncds",   
        0x14: "cdp",    
        0x16: "ncdt",   
        0x1b: "nccs",   
        0x1c: "cc",     
        0x1e: "ncs",    
        0x20: "nct",    
        0x28: "sqr",
        0x29: "dcpl",   
        0x2a: "dpct",   
        0x2d: "avsz3",  
        0x2e: "avsz4",  
        0x30: "rtpt",   
        0x3d: "gpf",
        0x3e: "gpl",
        0x3f: "ncct"
    }

    def __init__(self):
        super().__init__("psx_gte")

    def __call__(self, info):
        instr_bytes = info.read_memory(4)
        instr = int.from_bytes(instr_bytes, byteorder='little')

        if (instr & 0xFE000000) == 0x4A000000:
            opcode = instr & 0x1F

            name = self.gte_op_names.get(opcode, f"unknown_gte_{hex(opcode)}")

            return gdb.disassembler.DisassemblerResult(4, f"{name}\t")
        else:
            return None

gdb.disassembler.register_disassembler(GTEDisasm())
