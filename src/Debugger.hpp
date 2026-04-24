#pragma once

#include <vector>

#include "types.hpp"
#include "System.hpp"
#include "CPU.hpp"

namespace pse {

class Debugger {
public:
    Debugger(System& system);
    void run();

    void sys_run();
    void sys_breakpoint_set(u32 addr);
    void sys_breakpoint_remove(u32 addr);
    void sys_breakpoint_list();
    void cpu_dump() const;
    void cpu_setpc(u32 pc);
    void cpu_getpc();
    void mem_examine(u32 addr, u32 num) const;
    void mem_disassemble(u32 addr, u32 num) const;
    void mem_writefile(u32 addr, const std::string& filename);
    void bios_writefile(const std::string& filename);

    std::string mem_examine_get_str(u32 addr, u32 num_bytes, u32 bytes_per_line) const;


    static std::string disassemble(u32 pc, CPU::Instr i);

    class ParseError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

private:
    std::vector<u32> m_breakpoints;

    System& m_system;

};

    
};
