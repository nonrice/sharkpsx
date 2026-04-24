#pragma once

#include <vector>
#include <map>

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

    static std::string disassemble(u32 pc, CPU::Instr i);

    class ParseError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

private:
    std::vector<u32> m_breakpoints;
    
    struct Variable {
        std::string name;
        u32 val;
        bool reserved;
    };
    std::map<std::string, Variable> m_vars;
    u32 expand_var(const std::string& name);
    u32 eval_expr(const std::string& expr);
    u32 read_expr(std::istream& is);//more like read+eval expr. is it confusing?
    u32 read_hex(std::istream& is);
    u32 read_dec(std::istream& is);
    std::string read_str(std::istream& is);

    System& m_system;

};

    
};
