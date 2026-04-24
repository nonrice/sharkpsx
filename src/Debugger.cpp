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

#define HEX32 "{:#010x}"
#define HEX16 "{:#06x}"

namespace pse {

static void print_prompt(){
    std::cout << "> ";
}

template <typename... Args>
static void print(std::format_string<Args...> fmt, Args&&... args){
    std::cout << std::vformat(fmt.get(), std::make_format_args(args...));
}

template <typename... Args>
static void println(std::format_string<Args...> fmt, Args&&... args){
    std::cout << std::vformat(fmt.get(), std::make_format_args(args...)) << std::endl;
}

Debugger::Debugger(System& system) : m_system(system) {
    m_vars["pc"] = Debugger::Variable {
        .name = "pc",
        .reserved = true,
        .val = 0
    };

    println("SHARKPSX DEBUGGER (dev)");
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

void Debugger::cpu_dump() const {
    const CPU& cpu = m_system.m_cpu;

    print("pc: " HEX32 "\n", cpu.m_cur_pc);
    for (usize i=0; i<CPU::NUM_REGS; i++){
        print("r{}: " HEX32 "\n", i, cpu.m_regs[i]);
    }
    println("hi: " HEX32 "\n"
            "lo: " HEX32
            , cpu.m_hi, cpu.m_lo);
}

void Debugger::cpu_setpc(u32 pc) {
    m_system.m_cpu.set_pc(pc);
    println("Set pc to " HEX32, m_system.m_cpu.m_cur_pc);
}

void Debugger::mem_examine(u32 addr, u32 num) const {
    addr -= addr % 4;
    for (u32 i=0; i<num; i++){
        try {
            println(HEX32 ": " HEX32, addr, m_system.m_bus.read32(addr)); 
        } catch (Panic p){
            println("Failed to read word at " HEX32 ": {}", addr, p.what());
        }
        addr += 4;
    }
    std::cout.flush();
}

void Debugger::mem_disassemble(u32 addr, u32 num) const {
    addr -= addr % 4;
    for (u32 i=0; i<num; i++){
        try {
            u32 word = m_system.m_bus.read32(addr);
            println(HEX32 ": " HEX32 " {}",
                    addr, word, disassemble(addr, CPU::Instr{word}));
        } catch (Panic p){
            println("Failed to read word at " HEX32 ": {}", addr, p.what());
        }
        addr += 4;
    }
    std::cout.flush();
}

void Debugger::mem_writefile(u32 addr, const std::string& name) {
    std::ifstream is(name, std::ios::binary);

    if (!is.is_open()){
        println("File {} couldn't be opened", name);
        return;
    }

    is.seekg(0, is.end);
    u32 len = is.tellg();
    is.seekg(0, is.beg);

    is.read(reinterpret_cast<char*>(m_system.m_ram.m_data->data()) + addr, len);
    println("Wrote {} bytes starting at " HEX32, len, addr);
}

void Debugger::bios_writefile(const std::string& name){
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

    is.read(reinterpret_cast<char*>(m_system.m_bios_rom.m_data->data()), len);
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
        try {
            m_system.tick();
        } catch (Panic p) {
            println("System panicked: {}\nStopping", p.what());
            break;
        }

        if (std::find(m_breakpoints.begin(), m_breakpoints.end(), m_system.m_cpu.m_cur_pc) != m_breakpoints.end()){
            println("Reached breakpoint " HEX32 "\nStopping", m_system.m_cpu.m_cur_pc);
            break;
        }
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
    println("pc: " HEX32, m_system.m_cpu.m_cur_pc);
}

static u32 disas_calc_branch(u32 pc, s16 d){
    return static_cast<u32>(
        static_cast<s32>(pc) + 4 + 4 * static_cast<s32>(d)
    );
}

u32 Debugger::expand_var(const std::string& name){
    auto it = m_vars.find(name);
    if (it == m_vars.end()){
        throw Debugger::ParseError(std::format("Unknown variable {}", name));
    }

    Debugger::Variable v = it->second;

    if (v.reserved){
        if (v.name == "pc"){
            return m_system.m_cpu.m_cur_pc;
        }
    }

    return v.val;
}

std::string Debugger::disassemble(u32 pc, CPU::Instr i){
    if (i.val == 0){
        return "nop";
    }

    switch (i.primary_opcode()){
        case 0x00: 
            switch (i.secondary_opcode()){
                case 0x00: return std::format("sll r{} r{} {}", i.rd(), i.rt(), i.imm5());
                case 0x02: return std::format("srl r{} r{} {}", i.rd(), i.rt(), i.imm5());
                case 0x03: return std::format("sra r{} r{} {}", i.rd(), i.rt(), i.imm5());
                case 0x04: return std::format("sllv r{} r{} r{}", i.rd(), i.rt(), i.rs());
                case 0x06: return std::format("srlv r{} r{} r{}", i.rd(), i.rt(), i.rs());
                case 0x07: return std::format("srav r{} r{} r{}", i.rd(), i.rt(), i.rs());
                case 0x08: return std::format("jr r{}", i.rs());
                case 0x09: return std::format("jalr r{}", i.rs());
                case 0x0c: return std::format("syscall");
                case 0x0D: return std::format("break");
                case 0x10: return std::format("mfhi r{}", i.rd());
                case 0x11: return std::format("mthi r{}", i.rs());
                case 0x12: return std::format("mflo r{}", i.rd());
                case 0x13: return std::format("mtlo r{}", i.rs());
                case 0x18: return std::format("mult r{} r{}", i.rs(), i.rt());
                case 0x19: return std::format("multu r{} r{}", i.rs(), i.rt());
                case 0x1A: return std::format("div r{} r{}", i.rs(), i.rt());
                case 0x1B: return std::format("divu r{} r{}", i.rs(), i.rt());
                case 0x20: return std::format("add r{} r{} r{}", i.rd(), i.rs(), i.rt());
                case 0x21: return std::format("addu r{} r{} r{}", i.rd(), i.rs(), i.rt());
                case 0x22: return std::format("sub r{} r{} r{}", i.rd(), i.rs(), i.rt());
                case 0x23: return std::format("subu r{} r{} r{}", i.rd(), i.rs(), i.rt());
                case 0x24: return std::format("and r{} r{} r{}", i.rd(), i.rs(), i.rt());
                case 0x25: return std::format("or r{} r{} r{}", i.rd(), i.rs(), i.rt());
                case 0x26: return std::format("xor r{} r{} r{}", i.rd(), i.rs(), i.rt());
                case 0x27: return std::format("nor r{} r{} r{}", i.rd(), i.rs(), i.rt());
                case 0x2A: return std::format("slt r{} r{} r{}", i.rd(), i.rs(), i.rt());
                case 0x2B: return std::format("sltu r{} r{} r{}", i.rd(), i.rs(), i.rt());
            }
        case 0x01: 
            switch (i.rt()){
                case 0x00: return std::format("bltz r{} " HEX32,
                                   i.rs(), disas_calc_branch(pc, i.imm16_signed()));
                case 0x01: return std::format("bgez r{} " HEX32,
                                   i.rs(), disas_calc_branch(pc, i.imm16_signed()));
                case 0x10: return std::format("bltzal r{} " HEX32,
                                   i.rs(), disas_calc_branch(pc, i.imm16_signed()));
                case 0x11: return std::format("bgezal r{} " HEX32,
                                   i.rs(), disas_calc_branch(pc, i.imm16_signed()));
            }
        case 0x02: return std::format("j " HEX32,
                           ((pc + 4) & 0xF0000000) + 4 * i.imm26());
        case 0x03: return std::format("jal " HEX32,
                           ((pc + 4) & 0xF0000000) + 4 * i.imm26());
        case 0x04: return std::format("beq r{} r{} " HEX32,
                           i.rs(), i.rt(), disas_calc_branch(pc, i.imm16_signed()));
        case 0x05: return std::format("bne r{} r{} " HEX32,
                           i.rs(), i.rt(), disas_calc_branch(pc, i.imm16_signed()));
        case 0x06: return std::format("blez r{} " HEX32,
                           i.rs(), disas_calc_branch(pc, i.imm16_signed()));
        case 0x07: return std::format("bgtz r{} " HEX32,
                           i.rs(), disas_calc_branch(pc, i.imm16_signed()));
        case 0x08: return std::format("addi r{} r{} {}",
                           i.rt(), i.rs(), i.imm16_signed());
        case 0x09: return std::format("addiu r{} r{} {}",
                           i.rt(), i.rs(), i.imm16_signed());
        case 0x0a: return std::format("slti r{} r{} {}", i.rt(), i.rs(), i.imm16_signed());
        case 0x0b: return std::format("sltiu r{} r{} {}",
                           i.rt(), i.rs(),
                           static_cast<u32>(static_cast<s32>(i.imm16_signed())));
        case 0x0c: return std::format("andi r{} r{} " HEX16, i.rt(), i.rs(), i.imm16());
        case 0x0d: return std::format("ori r{} r{} " HEX16, i.rt(), i.rs(), i.imm16());
        case 0x0e: return std::format("xori r{} r{} " HEX16, i.rt(), i.rs(), i.imm16());
        case 0x0f: return std::format("lui r{} " HEX16, i.rt(), i.imm16());
        case 0x10: return std::format("cop0");
        case 0x11: return std::format("cop1");
        case 0x12: return std::format("cop2");
        case 0x13: return std::format("cop3");
        case 0x20: return std::format("lb r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x21: return std::format("lh r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x22: return std::format("lwl r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x23: return std::format("lw r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x24: return std::format("lbu r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x25: return std::format("lhu r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x26: return std::format("lwr r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x28: return std::format("sb r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x29: return std::format("sh r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x2a: return std::format("swl r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x2b: return std::format("sw r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
        case 0x2e: return std::format("swr r{} {}(r{})",
                           i.rt(), i.imm16_signed(), i.rs());
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

void Debugger::run(){
    while (true){
        print_prompt();

        std::string input;
        if (!std::getline(std::cin, input) || std::cin.eof()){
            break;
        }

        std::stringstream args(input);

        std::string cmd;
        if (!(args >> cmd)) {
            continue;
        }

        if (cmd == "quit" || cmd == "exit"){
            break;
        }
        
        try {
            if (cmd == "cpu"){
                std::string arg1;
                args >> arg1;
                if (arg1 == "dump"){
                    cpu_dump();
                } else if (arg1 == "setpc"){
                    u32 pc = read_hex(args);
                    cpu_setpc(pc);
                } else if (arg1 == "getpc"){
                    cpu_getpc();
                } else {
                    throw Debugger::ParseError("Unknown command");
                }
            } else if (cmd == "mem"){
                std::string arg1;
                args >> arg1;
                if (arg1 == "examine" || arg1 == "x"){
                    u32 addr = read_hex(args);
                    u32 num = read_dec(args);
                    mem_examine(addr, num);
                } else if (arg1 == "writefile"){
                    u32 addr = read_hex(args);
                    std::string name = read_str(args);
                    mem_writefile(addr, name);
                } else if (arg1 == "disassemble" || arg1 == "disas"){
                    u32 addr = read_hex(args);
                    u32 num = read_dec(args);
                    mem_disassemble(addr, num);
                } else {
                    throw Debugger::ParseError("Unknown command");
                }
            } else if (cmd == "bios"){
                std::string arg1;
                args >> arg1;
                if (arg1 == "writefile"){
                    std::string name = read_str(args);
                    bios_writefile(name);
                } else {
                    throw Debugger::ParseError("Unknown command");
                }
            } else if (cmd == "sys"){
                std::string arg1;
                args >> arg1;
                if (arg1 == "run"){
                    sys_run();
                } else if (arg1 == "breakpoint" || arg1 == "br"){
                    std::string arg2;
                    args >> arg2;
                    if (arg2 == "set"){
                        u32 addr = read_hex(args);
                        sys_breakpoint_set(addr);
                    } else if (arg2 == "remove"){
                        u32 addr = read_hex(args);
                        sys_breakpoint_remove(addr);
                    } else if (arg2 == "list"){
                        sys_breakpoint_list();
                    } else {
                        throw Debugger::ParseError("Unknown command");
                    }
                } else {
                    throw Debugger::ParseError("Unknown command");
                }
            } else {
                throw Debugger::ParseError("Unknown command");
            }
        } catch (Debugger::ParseError p) {
            println("Couldn't parse command: {}", p.what());
        }
    }
}

};

#undef HEX32
#undef HEX16
