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
    u32 read_expr(std::istream& is);
    u32 read_hex(std::istream& is);
    u32 read_dec(std::istream& is);
    std::string read_str(std::istream& is);

    enum class ParseMethod {
        Hex,
        Dec,
        Str
    };

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
    void process_method(
            std::istream& is,
            std::function<void(ParseMethodToType<Ps>...)> f
    ){
        std::tuple<ParseMethodToType<Ps>...> args{ read<Ps>(is)... };
        std::apply(f, args);
    }

    template <ParseMethod... Ps, typename F>
    auto io_bind_method(std::istream& is, F f){
        return [&is, f, this](){
            process_method<Ps...>(
                    is,
                    [f, this](ParseMethodToType<Ps>... args){
                        std::invoke(f, this, args...);
                    }
            );

        };
    }

    System& m_system;

};

    
};
