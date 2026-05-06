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
    void sys_breakpoint_list();
    void cpu_dump();
    void cpu_setpc(u32 pc);
    void cpu_getpc();
    void mem_examine(u32 addr, u32 num);
    void mem_disassemble(u32 addr, u32 num);
    void mem_writefile(u32 addr, std::string filename);
    void bios_writefile(std::string filename);
    void dec2hex(u32 d);
    void hex2dec(u32 h);
    void sys_watchpoint_set_read(u32 addr);
    void sys_watchpoint_set_write(u32 addr);
    void sys_watchpoint_list();
    void sys_watchpoint_remove(u32 addr);
    void sys_sideload_set(std::string filename);
    void sys_sideload_remove();

    static std::string disassemble(u32 pc, CPU::Instr i);
    // map reg number to the conventional name
    static std::string regname(u32 reg);

    void sideload(const std::string& path);

private:
    System& m_sys;
    std::vector<u32> m_breakpoints;

    struct Watchpoint {
        enum Kind {
            READ,
            WRITE
        };

        Kind kind;
        u32 addr;

        bool operator==(const Watchpoint& o) const;
    };
    std::vector<Watchpoint> m_watchpoints;
    std::vector<Watchpoint>::iterator find_watchpoint(Watchpoint w);

    std::optional<std::string> m_file_to_sideload;

    // process entire line of debugger console input
    // return true to quit
    bool eval_line(std::istream& is);

    // variable system for the debugger...
    //
    // In the future this will be a basis for an expression system
    // i.e. u can just write $(expr) and it will eval expr to give
    // a parameter you can pass to other functions
    //
    // Maybe not that useful... but cool
    // but for now, we can do stuff like $pc which is nice
    struct Variable {
        std::string name;
        u32 val;
        bool reserved;
    };
    std::map<std::string, Variable> m_vars;

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
    u32 expand_var(const std::string& name);
    u32 eval_expr(const std::string& expr);
    u32 read_expr(std::istream& is);
    u32 read_hex(std::istream& is);
    u32 read_dec(std::istream& is);
    std::string read_str(std::istream& is);

    // overengineered parsing stuff below

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
        // this is currently std::string
        // methods that take str also take std::string accordingly
        //
        // fine and simple but would copy the string, then
        //
        // this already took a while to figure out, messing with
        // move/reference stuff to eliminate the copy of what, like 20 chars
        // is not something iwant to spend more time on.... i'll try later 
        // because it might be easy though but for now i dont want to hink
        // about it
        using type = std::string;
    };

    template <ParseMethod P>
    using ParseMethodToType = typename ParseMethodMap<P>::type;

    // generic read which calls the specialized reads
    //
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
