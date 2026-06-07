import gdb
import gdb.disassembler
from enum import Enum

class GTEDisasm(gdb.disassembler.Disassembler):
    _op_names = {
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

    _mx_names = ["R", "L", "LR", "?3"]
    _v_names = ["V0", "V1", "V2", "IR"]
    _cv_names = ["TR", "BK", "?2", "0"]

    def __init__(self):
        super().__init__("psx_gte")

    def __call__(self, info):
        instr_bytes = info.read_memory(4)
        instr = int.from_bytes(instr_bytes, byteorder='little')

        if (instr & 0xFE000000) != 0x4A000000:
            return None # so just use the regular disassembler
        
        # is GTE code:
        opcode = instr & 0x1F
        sf = (instr >> 19) & 1
        lm = (instr >> 10) & 1

        name = self._op_names.get(opcode, f"gte_{hex(opcode)}")

        params = f"sf={ sf },lm={ lm }"

        if opcode == 0x12: # mvmva
            mx = (instr >> 17) & 0x3
            v = (instr >> 15) & 0x3 
            cv = (instr >> 13) & 0x3
            params += (
                f",mx={ self._mx_names[mx] },"
                f"v={ self._v_names[v] },"
                f"cv={ self._cv_names[cv] }"
                )

        return gdb.disassembler.DisassemblerResult(4, f"{name}\t{params}")

gdb.disassembler.register_disassembler(GTEDisasm())
