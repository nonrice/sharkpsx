#pragma once

#include <vector>
#include <map>
#include <format>

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
    void cpu_dump();
    void cpu_setpc(u32 pc);
    void cpu_getpc();
    void mem_examine(u32 addr, u32 num);
    void mem_disassemble(u32 addr, u32 num);
    void mem_writefile(u32 addr, std::string filename);
    void bios_writefile(std::string filename);

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
    u32 read_expr(std::istream& is);
    u32 read_hex(std::istream& is);
    u32 read_dec(std::istream& is);
    std::string read_str(std::istream& is);

    enum class ParseMethod {
        Hex,
        Dec,
        Str
    };
    using PM = ParseMethod;

    // This maps the parse methods to their underlying return types
    // at compile time, to extend u just add another specialization
    template <ParseMethod P>
    struct ParseMethodMap;
    template<> struct ParseMethodMap<ParseMethod::Hex> {
        using type = u32;
    };
    template<> struct ParseMethodMap<ParseMethod::Dec> {
        using type = u32;
    };
    template<> struct ParseMethodMap<ParseMethod::Str> {
        using type = std::string;
    };

    template <ParseMethod P>
    using ParseMethodToType = typename ParseMethodMap<P>::type;

    // generic read which calls the specialized reads
    // originally i only had the specialized ones
    // hence not using some constexpr if. but maybe it's better this way
    // These implementations r borderline specifications instead
    // hence i think keeping them in the header is better
    //
    // Other templates go into .cpp since they r private for debugger anyways
    template <ParseMethod P>
    ParseMethodToType<P> read(std::istream& is);
    template<> u32 read<ParseMethod::Hex>(std::istream& is){
        return read_hex(is);
    }
    template<> u32 read<ParseMethod::Dec>(std::istream& is){
        return read_dec(is);
    }
    template<> std::string read<ParseMethod::Str>(std::istream& is){
        return read_str(is);
    }

    template <ParseMethod... Ps>
    void process_cmd(
            std::istream& is,
            std::function<void(ParseMethodToType<Ps>...)> f
    );

    //me when 150 is useful!!!
    // gives back io bound cmd u can just call directly !
    template <ParseMethod... Ps, typename F>
    std::function<void(std::istream&)> io_bind_cmd(F f);

    std::map<std::string, std::function<void(std::istream&)>> cmds;

    template<ParseMethod... Ps, typename F>
    void register_cmd(const std::string & name, F f);

    void eval_line(std::istream& is);

    System& m_system;
};

    
};
