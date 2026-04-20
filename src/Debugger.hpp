#pragma once

#include "types.hpp"
#include "System.hpp"

namespace pse {

class Debugger {
public:
    Debugger(System& system);
    void run();

    void system_run();

    void cpu_dump() const;
    void cpu_setpc(u32 pc);
    std::string cpu_dump_get_str() const;

    void mem_examine_word(u32 addr, u32 num) const;
    u32 mem_writefile(u32 addr, const std::string& filename);
    std::string mem_examine_get_str(u32 addr, u32 num_bytes, u32 bytes_per_line) const;

private:
    System& m_system;
};
    
};
