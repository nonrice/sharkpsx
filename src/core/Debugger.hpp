#pragma once

#include <map>
#include <functional>

#include "types.hpp"
#include "System.hpp"
#include "CPU.hpp"
#include "BasicDebug.hpp"

namespace pse {

class Debugger {
public:
    Debugger(System& system);
    void run();
    void run_file(const std::string& path);

    // these r just cmd implementations
    // public beacuse i guess they are useful?
    // Though not too useful in a real project... since they just 
    // print to stdout
    //
    // Maybe could give Debugger it's own stream, have user decide
    // to attach to stdout or not
    //
    // Still not too useful though... i think
    void sys_run();
    void sys_breakpoint_set(u32 addr);
    void sys_breakpoint_remove(u32 addr);
    void cpu_dump();
    void cpu_setpc(u32 pc);
    void cpu_getpc();
    void mem_examine(u32 addr, u32 num);
    void mem_disassemble(u32 addr, u32 num);
    void bios_writefile(std::string filename);
    void dec2hex(u32 d);
    void hex2dec(u32 h);
    void sys_watchpoint_set_read(u32 addr);
    void sys_watchpoint_set_write(u32 addr);
    void sys_watchpoint_remove(u32 addr);
    void sys_sideload_set(std::string filename);
    void sys_sideload_remove();
    void server(u32 port);

    static std::string disassemble(u32 pc, CPU::Instr i);
    // map reg number to the conventional name
    static std::string regname(u32 reg);

private:
    BasicDebug m_dbg;

    // process entire line of debugger console input
    // return true to quit
    bool eval_line(std::istream& is);

    enum class ParseMethod {
        Hex,
        Dec,
        Str
    };
    using PM = ParseMethod;

    // usage
    // register_cmd<type1, type2, ...>("command", fn)
    // defines a command that has prototpe command type1 tpe2 ...
    // then feeds params into fn when called
    template<ParseMethod... Ps, typename F>
    void register_cmd(const std::string & name, F f);

    // parsing related stuff/methods
    class ParseError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };
    u32 read_hex(std::istream& is);
    u32 read_dec(std::istream& is);
    std::string read_str(std::istream& is);

    // overengineered parsing stuff below

    // This maps the parse methods to their underlying return types
    // at compile time, to extend u ~~just add another specialization~~
    // (^ refers to old template specialization trick, seems more standard? But
    // Makes this uglier than it already is so..)
    //
    // Here, just nest to add more types
    template <ParseMethod P>
    using ParseMethodToType = std::conditional_t<P == ParseMethod::Str, std::string, u32>;

    // generic read which calls the specialized reads
    //
    // originally i only had the specialized ones
    // hence not using some constexpr if. but maybe it's better this way
    // These implementations r borderline specifications instead
    // hence i think keeping them in the header is better
    //
    // Other templates go into .cpp since they r private for debugger anyways
    template <ParseMethod P>
    ParseMethodToType<P> read(std::istream& is) {
        static_assert(P == ParseMethod::Hex ||
                P == ParseMethod::Dec ||
                P == ParseMethod::Str);
        if constexpr (P == ParseMethod::Hex){
            return read_hex(is);
        } else if constexpr (P == ParseMethod::Dec){
            return read_dec(is);
        } else if constexpr (P == ParseMethod::Str){
            return read_str(is);
        }
    }

    // runs command by calling the correct io methods, automatically!
    template <ParseMethod... Ps>
    void process_cmd(
            std::istream& is,
            std::function<void(ParseMethodToType<Ps>...)> f
    );

    using IOBoundCmd = std::function<void(std::istream&)>;
    std::map<std::string, IOBoundCmd> m_cmds;

    //me when 150 is useful!!!
    // gives back io bound cmd u can just call directly !
    template <ParseMethod... Ps, typename F>
    IOBoundCmd io_bind_cmd(F f);
};

    
};
