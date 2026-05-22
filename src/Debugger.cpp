#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <format>
#include <exception>
#include <atomic>
#include <csignal>

#include "types.hpp"
#include "Panic.hpp"
#include "Debugger.hpp"
#include "CPU.hpp"
#include "logging.hpp"

namespace pse {

template <typename... Args>
static void print(std::format_string<Args...> fmt, Args&&... args){
    std::cout << std::vformat(fmt.get(), std::make_format_args(args...));
}

template <typename... Args>
static void println(std::format_string<Args...> fmt, Args&&... args){
    std::cout << std::vformat(fmt.get(), std::make_format_args(args...)) << std::endl;
}

static void print_prompt(){
    std::cout << "> ";
}

void Debugger::run_file(const std::string& path){
    std::ifstream in(path);

    if (!in.is_open()){
        println("Couldn't open {}", path);
        return;
    }

    std::string input;
    while (!in.eof() && std::getline(in, input)){
        std::stringstream args(input);

        bool quit = false;
        try {
            quit = eval_line(args);
        } catch (Debugger::ParseError p) {
            println("Couldn't parse command: {}", p.what());
            break;
        }
        
        if (quit){
            return;
        }
    }

    // if no quit then we go to the console
    run();
}

void Debugger::run(){
    while (true){
        print_prompt();

        std::string input;
        if (!std::getline(std::cin, input) || std::cin.eof()){
            break;
        }

        std::stringstream args(input);

        bool quit = false;
        try {
            quit = eval_line(args);
        } catch (Debugger::ParseError p) {
            println("Couldn't parse command: {}", p.what());
        }

        if (quit){
            return;
        }
    }
}

Debugger::Debugger(System& system) : m_sys(system) {
    register_cmd<>("cpu dump", &Debugger::cpu_dump);
    register_cmd<PM::Hex>("cpu setpc", &Debugger::cpu_setpc);
    register_cmd<PM::Hex, PM::Dec>("mem examine", &Debugger::mem_examine);
    register_cmd<PM::Hex, PM::Dec>("mem x", &Debugger::mem_examine);
    register_cmd<PM::Hex, PM::Str>("mem writefile", &Debugger::mem_writefile);
    register_cmd<PM::Hex, PM::Dec>("mem disassemble", &Debugger::mem_disassemble);
    register_cmd<PM::Hex, PM::Dec>("mem disas", &Debugger::mem_disassemble);
    register_cmd<PM::Str>("bios writefile", &Debugger::bios_writefile);
    register_cmd<>("sys run", &Debugger::sys_run);
    register_cmd<PM::Hex>("sys breakpoint set", &Debugger::sys_breakpoint_set);
    register_cmd<PM::Hex>("sys breakpoint remove", &Debugger::sys_breakpoint_remove);
    register_cmd<>("sys breakpoint list", &Debugger::sys_breakpoint_list);
    register_cmd<PM::Hex>("sys br set", &Debugger::sys_breakpoint_set);
    register_cmd<PM::Hex>("sys br remove", &Debugger::sys_breakpoint_remove);
    register_cmd<>("sys br list", &Debugger::sys_breakpoint_list);
    register_cmd<PM::Hex>("sys watchpoint set read", &Debugger::sys_watchpoint_set_read);
    register_cmd<PM::Hex>("sys watchpoint set write", &Debugger::sys_watchpoint_set_write);
    register_cmd<>("sys watchpoint list", &Debugger::sys_watchpoint_list);
    register_cmd<PM::Hex>("sys watchpoint remove", &Debugger::sys_watchpoint_remove);
    register_cmd<PM::Str>("sys sideload set", &Debugger::sys_sideload_set);
    register_cmd<>("sys sideload remove", &Debugger::sys_sideload_remove);
    register_cmd<PM::Hex>("hex2dec", &Debugger::hex2dec);
    register_cmd<PM::Hex>("h2d", &Debugger::hex2dec);
    register_cmd<PM::Dec>("dec2hex", &Debugger::dec2hex);
    register_cmd<PM::Dec>("d2h", &Debugger::dec2hex);


    m_vars["pc"] = Debugger::Variable {
        .name = "pc",
        .reserved = true,
        .val = 0
    };

    println("SHARKPSX DEBUGGER (dev)");
}

bool Debugger::eval_line(std::istream& is){
    std::string name;
    while (!is.eof()){
        std::string tok;
        is >> tok;
        if (tok.empty()){
            //it's just a newline,or some whitespace
            return false;
        }

        if (tok == "quit"){
            return true;
        }

        if (!name.empty()){
            name += " ";
        }
        name += tok;

        if (m_cmds.find(name) != m_cmds.end()){
            m_cmds[name](is);
            return false;
        }
    }

    throw Debugger::ParseError("Unknown command");
}

template <Debugger::ParseMethod... Ps>
void Debugger::process_cmd(
        std::istream& is,
        std::function<void(Debugger::ParseMethodToType<Ps>...)> f
){
    std::tuple<Debugger::ParseMethodToType<Ps>...> args{ read<Ps>(is)... };
    std::apply(f, args);
}

template <Debugger::ParseMethod... Ps, typename F>
std::function<void(std::istream&)> Debugger::io_bind_cmd(F f){
    return [f, this](std::istream& is){
        process_cmd<Ps...>(
                is,
                [f, this](ParseMethodToType<Ps>... args){
                    std::invoke(f, this, args...);
                }
        );
    };
}

template<Debugger::ParseMethod... Ps, typename F>
void Debugger::register_cmd(const std::string & name, F f){
    if (m_cmds.find(name) != m_cmds.end()){
        throw std::runtime_error(std::format("Command {} exists already", name));
    }

    m_cmds[name] = io_bind_cmd<Ps...>(f);
}

bool Debugger::Watchpoint::operator==(const Watchpoint& o) const{
    return std::tie(kind, addr) == std::tie(o.kind, o.addr);
}

std::vector<Debugger::Watchpoint>::iterator
Debugger::find_watchpoint(Debugger::Watchpoint w){
    auto it = std::find(m_watchpoints.begin(), m_watchpoints.end(), w);
    return it;
}

void Debugger::sys_watchpoint_set_read(u32 addr){
    Watchpoint w{.kind = Watchpoint::READ, .addr = addr};
    
    if (find_watchpoint(w) != m_watchpoints.end()){
        println("Read watchpoint at " HEX32 " exists already", addr);
        return;
    }

    m_watchpoints.push_back(w);
}

void Debugger::sys_watchpoint_set_write(u32 addr){
    Watchpoint w{.kind = Watchpoint::WRITE, .addr = addr};
    
    if (find_watchpoint(w) != m_watchpoints.end()){
        println("Write watchpoint at " HEX32 " exists already", addr);
        return;
    }

    m_watchpoints.push_back(w);
}

void Debugger::sys_watchpoint_list(){
    for (auto [k, x] : m_watchpoints){
        switch (k) {
            case Watchpoint::READ:
                print("Read ");
                break;
            case Watchpoint::WRITE:
                print("Write ");
                break;
        }

        println(HEX32, x);
    }
}

void Debugger::sys_watchpoint_remove(u32 addr){
    bool found = false;
    std::vector<Watchpoint>::iterator it;
    while ((it = std::find_if(m_watchpoints.begin(), m_watchpoints.end(), 
                [&](const Watchpoint& w){
                    return w.addr == addr;
                })) != m_watchpoints.end()){
        found = true;
        m_watchpoints.erase(it);
    }

    if (!found){
        println("Watchpoint at " HEX32 " does not exist", addr);
    }
}

static bool next_is_expr(std::istream& is){
    is >> std::ws;
    return is.peek() == '$';
}

u32 Debugger::eval_expr(const std::string& expr){
    return expand_var(expr);
}

u32 Debugger::read_expr(std::istream& is){
    std::string s;
    if (!next_is_expr(is)){
        throw Debugger::ParseError("Expected string");
    }

    is.ignore(1);
    is >> s;
    return eval_expr(s);
}

u32 Debugger::read_hex(std::istream& is){
    u32 x;
    if (next_is_expr(is)){
        return read_expr(is);
    }

    if (!(is >> std::hex >> x >> std::dec)){
        throw Debugger::ParseError("Expected hex value");
    }
    return x;
}

u32 Debugger::read_dec(std::istream& is){
    u32 x;
    if (next_is_expr(is)){
        return read_expr(is);
    }

    if (!(is >> x)){
        throw Debugger::ParseError("Expected integer value");
    }
    return x;
}


std::string Debugger::read_str(std::istream& is){
    std::string s;
    if (!(is >> std::quoted(s))){
        throw Debugger::ParseError("Expected string");
    }

    return s;
}

void Debugger::cpu_dump() {
    const CPU& cpu = m_sys.m_cpu;

    print("pc: " HEX32 "\n", cpu.m_cur_pc);
    for (usize i=0; i<CPU::NUM_REGS; i++){
        print("{}: " HEX32 "\n", regname(i), cpu.m_regs[i]);
    }
    println("hi: " HEX32 "\n"
            "lo: " HEX32
            , cpu.m_hi, cpu.m_lo);
}

void Debugger::cpu_setpc(u32 pc) {
    m_sys.m_cpu.set_pc(pc);
    println("Set pc to " HEX32, m_sys.m_cpu.m_cur_pc);
}

static char conv_char(u8 x){
    if (x < 33 || x > 126){
        return '.';
    }
    return x;
}

void Debugger::mem_examine(u32 addr, u32 num) {
    addr -= addr % 4;
    for (u32 i=0; i<num; i++){
        u32 word;
        try {
            word = m_sys.m_bus.read32(addr);
        } catch (Panic p){
            println("Failed to read word at " HEX32 ": {}", addr, p.what());
        }
        char c1 = conv_char(word);
        char c2 = conv_char(word >> 8);
        char c3 = conv_char(word >> 16);
        char c4 = conv_char(word >> 24);
        println(HEX32 ": " HEX32 " {}{}{}{}",
                addr, m_sys.m_bus.read32(addr),
                c1, c2, c3, c4);
        addr += 4;
    }
    std::cout.flush();
}

void Debugger::sys_sideload_set(std::string filename){
    m_file_to_sideload = filename;
}

void Debugger::sys_sideload_remove(){
    m_file_to_sideload = std::nullopt;
}

void Debugger::mem_disassemble(u32 addr, u32 num) {
    addr -= addr % 4;
    for (u32 i=0; i<num; i++){
        try {
            u32 word = m_sys.m_bus.read32(addr);
            println(HEX32 ": " HEX32 " {}",
                    addr, word, disassemble(addr, CPU::Instr{word}));
        } catch (Panic p){
            println("Failed to read word at " HEX32 ": {}", addr, p.what());
        }
        addr += 4;
    }
    std::cout.flush();
}

void Debugger::mem_writefile(u32 addr, std::string name) {
    std::ifstream is(name, std::ios::binary);

    if (!is.is_open()){
        println("File {} couldn't be opened", name);
        return;
    }

    is.seekg(0, is.end);
    u32 len = is.tellg();
    is.seekg(0, is.beg);

    is.read(reinterpret_cast<char*>(m_sys.m_ram.m_data.get()) + addr, len);
    println("Wrote {} bytes starting at " HEX32, len, addr);
}

void Debugger::bios_writefile(std::string name){
    std::ifstream is(name, std::ios::binary);

    if (!is.is_open()){
        println("File {} couldn't be opened", name);
        return;
    }

    is.seekg(0, is.end);
    u32 len = is.tellg();
    is.seekg(0, is.beg);

    if (len > BIOSROM::SIZE){
        println("File too big");
        return;
    }

    is.read(reinterpret_cast<char*>(m_sys.m_bios_rom.m_data.get()), len);
    println("Wrote {} bytes into BIOS ROM", len);
}

static std::atomic<bool> pending_sigint{false};

static void sigint_handler([[maybe_unused]] int signal){
    pending_sigint = true;
}

void Debugger::sys_run(){
    println("Running, ^C to stop");
    std::signal(SIGINT, sigint_handler);

    pending_sigint = false;
    while (!pending_sigint){
        if (m_sys.m_cpu.m_cur_pc == 0x80030000){ // actual shell addr... 
            if (m_file_to_sideload){
                try {
                    sideload(*m_file_to_sideload);
                } catch (std::runtime_error e){
                    println("sharkpsx sideload: Error, {}\nStopping", e.what());
                    break;
                }
            }
        }

        try {
            m_sys.tick();
        } catch (Panic p) {
            println("System panicked: {}\nStopping", p.what());
            break;
        }

        if (std::find(m_breakpoints.begin(), m_breakpoints.end(), m_sys.m_cpu.m_cur_pc) != m_breakpoints.end()){
            println("Reached breakpoint " HEX32 "\nStopping", m_sys.m_cpu.m_cur_pc);
            break;
        }

        if (m_sys.m_bus.m_read_addr){
            u32 addr = *m_sys.m_bus.m_read_addr;
            m_sys.m_bus.m_read_addr = std::nullopt;
            if (find_watchpoint({.kind = Watchpoint::READ, .addr = addr})
                    != m_watchpoints.end()){
                println("Triggered read watchpoint at " HEX32 "\nStopping", addr);
                break;
            }
        }

        if (m_sys.m_bus.m_write_addr){
            u32 addr = *m_sys.m_bus.m_write_addr;
            m_sys.m_bus.m_write_addr = std::nullopt;
            if (find_watchpoint({.kind = Watchpoint::WRITE, .addr = addr})
                    != m_watchpoints.end()){
                println("Triggered write watchpoint at " HEX32 "\nStopping", addr);
                break;
            }
        }
    }
    if (pending_sigint){
        print("\n");
    }
    std::signal(SIGINT, SIG_DFL);

    pending_sigint = false;
}

void Debugger::sys_breakpoint_set(u32 addr){
    auto it = std::find(m_breakpoints.begin(), m_breakpoints.end(), addr);
    if (it != m_breakpoints.end()){
        println("Breakpoint " HEX32 " already exists", addr);
        return;
    }

    m_breakpoints.push_back(addr);
    return;
}

void Debugger::sys_breakpoint_remove(u32 addr){
    auto it = std::find(m_breakpoints.begin(), m_breakpoints.end(), addr);
    if (it == m_breakpoints.end()){
        println("Breakpoint " HEX32 " doesn't exist\n", addr);
        return;
    }

    m_breakpoints.erase(it);
    return;
}

void Debugger::sys_breakpoint_list(){
    for (auto x : m_breakpoints){
        print(HEX32 "\n", x);
    }
    std::cout.flush();
}

void Debugger::cpu_getpc(){
    println("pc: " HEX32, m_sys.m_cpu.m_cur_pc);
}

static u32 disas_calc_branch(u32 pc, s16 d){
    return static_cast<u32>(
        static_cast<s32>(pc) + 4 + 4 * static_cast<s32>(d)
    );
}

void Debugger::dec2hex(u32 d){
    println(HEX32, d);
}

void Debugger::hex2dec(u32 h){
    println("{}", h);
}

u32 Debugger::expand_var(const std::string& name){
    auto it = m_vars.find(name);
    if (it == m_vars.end()){
        throw Debugger::ParseError(std::format("Unknown variable {}", name));
    }

    Debugger::Variable v = it->second;

    if (v.reserved){
        if (v.name == "pc"){
            return m_sys.m_cpu.m_cur_pc;
        }
    }

    return v.val;
}

template <typename T>
static T is_read(std::istream& is){
    T val;
    if (!is.read(reinterpret_cast<char*>(&val), sizeof(val))){
        throw std::runtime_error("failed to read");
    }

    // i don't think this will ever actually be used, for obvious reasons
    // could bump to cpp23 for generic byteswap...
    if constexpr (std::endian::native == std::endian::big){
        if constexpr (sizeof(T) == 2){ 
            val = __builtin_byteswap16(val);
        } else if constexpr (sizeof(T) == 4){
            val = __builtin_byteswap32(val);
        } else if constexpr (sizeof(T) == 8){
            val = __builtin_byteswap64(val);
        } else {
            throw std::runtime_error("fuk you");
        }
    }
    return val;
}

void Debugger::sideload(const std::string& path){
    std::ifstream is(path, std::ios::binary);

    if (!is.is_open()){
        throw std::runtime_error(std::format("Couldn't open file {}", path));
    }

    u64 magic_tag = is_read<u64>(is);
    if (magic_tag != 0x45584520582d5350ULL){//"PS-X EXE"
        println("{}", magic_tag);
        throw std::runtime_error(std::format("Trying to sideload malformed psexe"));
    }

    is.ignore(4);
    is.ignore(4);
    u32 start_pc = is_read<u32>(is); // 0x10
    is.ignore(4);
    u32 dest_addr = is_read<u32>(is); // 0x18
    u32 len = is_read<u32>(is); // 0x1c
    is.ignore(4);
    is.ignore(4);
    is.ignore(4);
    is.ignore(4);
    u32 sp = is_read<u32>(is); //0x30
    is.seekg(0x800);


    println("sharkpsx sideload: Successfully parsed PSEXE header!");
    println("  start_pc: " HEX32, start_pc);
    println("  dest_addr: " HEX32, dest_addr);
    println("  len: {} bytes", len);
    println("  sp: " HEX32, sp);

    //TODO add check for buffer overflow...
    dest_addr &= 0x1FFFFFFF;
    is.read(reinterpret_cast<char*>(m_sys.m_ram.m_data.get()) + dest_addr, len);
    m_sys.m_cpu.set_pc(start_pc);
    if (sp != 0){
        m_sys.m_cpu.m_regs[CPU::SP] = sp;
    }
    m_sys.m_cpu.m_regs[CPU::RA] = 0xFFFFFFFF;// openbios crashes after shell returns anyways so...
    println("sharkpsx sideload: Loaded {} bytes", len);
}

std::string Debugger::regname(u32 reg){
    switch (reg){
        case 0: return "$r0";
        case 1: return "$at";
        case 28: return "$gp";
        case 29: return "$sp";
        case 30: return "$r30";
        case 31: return "$ra";
    }

    if (reg <= 3){
        return std::format("$v{}", reg-2);
    } else if (reg <= 7){
        return std::format("$a{}", reg-4);
    } else if (reg <= 15){
        return std::format("$t{}", reg-8);
    } else if (reg <= 23){
        return std::format("$s{}", reg-16);
    } else if (reg <= 25){
        return std::format("$t{}", reg-24 + 8);
    } else if (reg <= 27){
        return std::format("$k{}", reg-26);
    }

    return "$??";
}

std::string Debugger::disassemble(u32 pc, CPU::Instr i){
    if (i.val == 0){
        return "nop";
    }

    switch (i.primary){
        case 0x00: 
            switch (i.secondary){
                case 0x00: return std::format(
                                   "sll {} {} {}",
                                   regname(i.rd), regname(i.rt), i.imm5);
                case 0x02: return std::format(
                                   "srl {} {} {}",
                                   regname(i.rd), regname(i.rt), i.imm5);
                case 0x03: return std::format(
                                   "sra {} {} {}",
                                   regname(i.rd), regname(i.rt), i.imm5);
                case 0x04: return std::format(
                                   "sllv {} {} {}",
                                   regname(i.rd), regname(i.rt), regname(i.rs));
                case 0x06: return std::format(
                                   "srlv {} {} {}",
                                   regname(i.rd), regname(i.rt), regname(i.rs));
                case 0x07: return std::format(
                                   "srav {} {} {}",
                                   regname(i.rd), regname(i.rt), regname(i.rs));
                case 0x08: return std::format("jr {}", regname(i.rs));
                case 0x09: return std::format("jalr {}", regname(i.rs));
                case 0x0c: return std::format("syscall");
                case 0x0D: return std::format("break");
                case 0x10: return std::format("mfhi {}", regname(i.rd));
                case 0x11: return std::format("mthi {}", regname(i.rs));
                case 0x12: return std::format("mflo {}", regname(i.rd));
                case 0x13: return std::format("mtlo {}", regname(i.rs));
                case 0x18: return std::format(
                                   "mult {} {}",
                                   regname(i.rs), regname(i.rt));
                case 0x19: return std::format(
                                   "multu {} {}",
                                   regname(i.rs), regname(i.rt));
                case 0x1A: return std::format(
                                   "div {} {}",
                                   regname(i.rs), regname(i.rt));
                case 0x1B: return std::format(
                                   "divu {} {}",
                                   regname(i.rs), regname(i.rt));
                case 0x20: return std::format(
                                   "add {} {} {}",
                                   regname(i.rd), regname(i.rs), regname(i.rt));
                case 0x21: return std::format(
                                   "addu {} {} {}",
                                   regname(i.rd), regname(i.rs), regname(i.rt));
                case 0x22: return std::format(
                                   "sub {} {} {}",
                                   regname(i.rd), regname(i.rs), regname(i.rt));
                case 0x23: return std::format(
                                   "subu {} {} {}",
                                   regname(i.rd), regname(i.rs), regname(i.rt));
                case 0x24: return std::format(
                                   "and {} {} {}",
                                   regname(i.rd), regname(i.rs), regname(i.rt));
                case 0x25: return std::format(
                                   "or {} {} {}",
                                   regname(i.rd), regname(i.rs), regname(i.rt));
                case 0x26: return std::format(
                                   "xor {} {} {}",
                                   regname(i.rd), regname(i.rs), regname(i.rt));
                case 0x27: return std::format(
                                   "nor {} {} {}",
                                   regname(i.rd), regname(i.rs), regname(i.rt));
                case 0x2A: return std::format(
                                   "slt {} {} {}",
                                   regname(i.rd), regname(i.rs), regname(i.rt));
                case 0x2B: return std::format(
                                   "sltu {} {} {}",
                                   regname(i.rd), regname(i.rs), regname(i.rt));
            }
        case 0x01: 
            switch (i.rt){
                case 0x00: return std::format("bltz {} " HEX32,
                                   regname(i.rs), disas_calc_branch(pc, static_cast<s16>(i.imm16)));
                case 0x01: return std::format("bgez {} " HEX32,
                                   regname(i.rs), disas_calc_branch(pc, static_cast<s16>(i.imm16)));
                case 0x10: return std::format("bltzal {} " HEX32,
                                   regname(i.rs), disas_calc_branch(pc, static_cast<s16>(i.imm16)));
                case 0x11: return std::format("bgezal {} " HEX32,
                                   regname(i.rs), disas_calc_branch(pc, static_cast<s16>(i.imm16)));
            }
        case 0x02: return std::format("j " HEX32,
                           ((pc + 4) & 0xF0000000) + 4 * i.imm26);
        case 0x03: return std::format("jal " HEX32,
                           ((pc + 4) & 0xF0000000) + 4 * i.imm26);
        case 0x04: return std::format("beq {} {} " HEX32,
                           regname(i.rs), regname(i.rt), disas_calc_branch(pc, static_cast<s16>(i.imm16)));
        case 0x05: return std::format("bne {} {} " HEX32,
                           regname(i.rs), regname(i.rt), disas_calc_branch(pc, static_cast<s16>(i.imm16)));
        case 0x06: return std::format("blez {} " HEX32,
                           regname(i.rs), disas_calc_branch(pc, static_cast<s16>(i.imm16)));
        case 0x07: return std::format("bgtz {} " HEX32,
                           regname(i.rs), disas_calc_branch(pc, static_cast<s16>(i.imm16)));
        case 0x08: return std::format("addi {} {} {}",
                           regname(i.rt), regname(i.rs), static_cast<s16>(i.imm16));
        case 0x09: return std::format("addiu {} {} {}",
                           regname(i.rt), regname(i.rs), static_cast<s16>(i.imm16));
        case 0x0a: return std::format("slti {} {} {}", regname(i.rt), regname(i.rs), static_cast<s16>(i.imm16));
        case 0x0b: return std::format("sltiu {} {} {}",
                           regname(i.rt), regname(i.rs),
                           static_cast<u32>(static_cast<s32>(static_cast<s16>(i.imm16))));
        case 0x0c: return std::format("andi {} {} " HEX16, regname(i.rt), regname(i.rs), i.imm16);
        case 0x0d: return std::format("ori {} {} " HEX16, regname(i.rt), regname(i.rs), i.imm16);
        case 0x0e: return std::format("xori {} {} " HEX16, regname(i.rt), regname(i.rs), i.imm16);
        case 0x0f: return std::format("lui {} " HEX16, regname(i.rt), i.imm16);
        case 0x10: return std::format("cop0");
        case 0x11: return std::format("cop1");
        case 0x12: return std::format("cop2");
        case 0x13: return std::format("cop3");
        case 0x20: return std::format("lb {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x21: return std::format("lh {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x22: return std::format("lwl {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x23: return std::format("lw {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x24: return std::format("lbu {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x25: return std::format("lhu {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x26: return std::format("lwr {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x28: return std::format("sb {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x29: return std::format("sh {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x2a: return std::format("swl {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x2b: return std::format("sw {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x2e: return std::format("swr {} {}({})",
                           regname(i.rt), static_cast<s16>(i.imm16), regname(i.rs));
        case 0x30: return std::format("lwc0");
        case 0x31: return std::format("lwc1");
        case 0x32: return std::format("lwc2");
        case 0x33: return std::format("lwc3");
        case 0x38: return std::format("swc0");
        case 0x39: return std::format("swc1");
        case 0x3a: return std::format("swc2");
        case 0x3b: return std::format("swc3");
        default: return "unknown";
    };

}

};
