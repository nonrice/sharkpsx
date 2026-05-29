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

Debugger::Debugger(System& system) : m_dbg(BasicDebug(system)) {
    register_cmd<>("cpu dump", &Debugger::cpu_dump);
    register_cmd<PM::Hex>("cpu setpc", &Debugger::cpu_setpc);
    register_cmd<PM::Hex, PM::Dec>("mem examine", &Debugger::mem_examine);
    register_cmd<PM::Hex, PM::Dec>("mem x", &Debugger::mem_examine);
    register_cmd<PM::Hex, PM::Dec>("mem disassemble", &Debugger::mem_disassemble);
    register_cmd<PM::Hex, PM::Dec>("mem disas", &Debugger::mem_disassemble);
    register_cmd<PM::Str>("bios writefile", &Debugger::bios_writefile);
    register_cmd<>("sys run", &Debugger::sys_run);
    register_cmd<PM::Hex>("sys breakpoint set", &Debugger::sys_breakpoint_set);
    register_cmd<PM::Hex>("sys breakpoint remove", &Debugger::sys_breakpoint_remove);
    register_cmd<PM::Hex>("sys br set", &Debugger::sys_breakpoint_set);
    register_cmd<PM::Hex>("sys br remove", &Debugger::sys_breakpoint_remove);
    register_cmd<PM::Hex>("sys watchpoint set read", &Debugger::sys_watchpoint_set_read);
    register_cmd<PM::Hex>("sys watchpoint set write", &Debugger::sys_watchpoint_set_write);
    register_cmd<PM::Hex>("sys watchpoint remove", &Debugger::sys_watchpoint_remove);
    register_cmd<PM::Str>("sys sideload set", &Debugger::sys_sideload_set);
    register_cmd<>("sys sideload remove", &Debugger::sys_sideload_remove);
    register_cmd<PM::Hex>("hex2dec", &Debugger::hex2dec);
    register_cmd<PM::Hex>("h2d", &Debugger::hex2dec);
    register_cmd<PM::Dec>("dec2hex", &Debugger::dec2hex);
    register_cmd<PM::Dec>("d2h", &Debugger::dec2hex);


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

static std::vector<u8> file_into_vec(std::string name){
    std::ifstream file(name, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + name);
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg); 

    std::vector<u8> buffer(fileSize);

    if (file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return buffer;
    } else {
        throw std::runtime_error("Failed to read the file contents");
    }
}


u32 Debugger::read_hex(std::istream& is){
    u32 x;
    if (!(is >> std::hex >> x >> std::dec)){
        throw Debugger::ParseError("Expected hex value");
    }
    return x;
}

u32 Debugger::read_dec(std::istream& is){
    u32 x;
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

void Debugger::sys_watchpoint_set_read(u32 addr){
    m_dbg.set_watchpoint_read(addr);
}

void Debugger::sys_watchpoint_set_write(u32 addr){
    m_dbg.set_watchpoint_write(addr);
}

void Debugger::sys_watchpoint_remove(u32 addr){
    m_dbg.remove_watchpoint_read(addr);
    m_dbg.remove_watchpoint_write(addr);
}

void Debugger::cpu_dump() {
    auto regs = m_dbg.dump_regs();

    print("pc: " HEX32 "\n", regs.pc);
    for (usize i=0; i<regs.gp.size(); i++){
        print("{}: " HEX32 "\n", regname(i), regs.gp[i]);
    }
    println("hi: " HEX32 "\n"
            "lo: " HEX32
            , regs.hi, regs.lo);
}

void Debugger::cpu_setpc(u32 pc) {
    m_dbg.set_pc(pc);
    println("Set pc to " HEX32, pc);
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
            word = m_dbg.read32(addr);
        } catch (Panic p){
            println("Failed to read word at " HEX32 ": {}", addr, p.what());
        }
        char c1 = conv_char(word);
        char c2 = conv_char(word >> 8);
        char c3 = conv_char(word >> 16);
        char c4 = conv_char(word >> 24);
        println(HEX32 ": " HEX32 " {}{}{}{}",
                addr, word,
                c1, c2, c3, c4);
        addr += 4;
    }
    std::cout.flush();
}

void Debugger::sys_sideload_set(std::string filename){
    m_dbg.set_sideload(file_into_vec(filename));
}

void Debugger::sys_sideload_remove(){
    m_dbg.remove_sideload();
}

void Debugger::mem_disassemble(u32 addr, u32 num) {
    addr -= addr % 4;
    for (u32 i=0; i<num; i++){
        try {
            u32 word = m_dbg.read32(addr);
            println(HEX32 ": " HEX32 " {}",
                    addr, word, disassemble(addr, CPU::Instr{word}));
        } catch (Panic p){
            println("Failed to read word at " HEX32 ": {}", addr, p.what());
        }
        addr += 4;
    }
    std::cout.flush();
}


void Debugger::bios_writefile(std::string name){
    m_dbg.set_biosrom(file_into_vec(name));
}

void Debugger::sys_run(){
    BasicDebug::StopReason s = m_dbg.cont();

    if (s.reason == BasicDebug::StopReason::PANIC){
        println("System panicked: {}", s.msg);
        println("Stopping");
    }
}

void Debugger::sys_breakpoint_set(u32 addr){
    m_dbg.set_breakpoint(addr);
}

void Debugger::sys_breakpoint_remove(u32 addr){
    if (!m_dbg.remove_breakpoint(addr)){
        println("Breakpoint " HEX32 " doesn't exist\n", addr);
    }
}

void Debugger::cpu_getpc(){
    println("pc: " HEX32, m_dbg.dump_regs().pc);
}

void Debugger::dec2hex(u32 d){
    println(HEX32, d);
}

void Debugger::hex2dec(u32 h){
    println("{}", h);
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

static u32 disas_calc_branch(u32 pc, s16 d){
    return static_cast<u32>(
        static_cast<s32>(pc) + 4 + 4 * static_cast<s32>(d)
    );
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
