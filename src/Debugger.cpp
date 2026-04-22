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

namespace pse {

static void print_prompt(){
    std::cout << "> ";
}

Debugger::Debugger(System& system) : m_system(system) {
    std::cout << "PSXEMU DEBUGGER (dev)\n"
        "Enter \"help\" for help\n";
}

static u32 read_hex(std::istream& is){
    u32 x;
    if (!(is >> std::hex >> x >> std::dec)){
        throw Debugger::ParseError("Expected hex value");
    }
    return x;
}

static u32 read_dec(std::istream& is){
    u32 x;
    if (!(is >> x)){
        throw Debugger::ParseError("Expected integer value");
    }
    return x;
}

static std::string read_str(std::istream& is){
    std::string s;
    if (!(is >> std::quoted(s))){
        if (!(is >> s)){
            throw Debugger::ParseError("Expected string");
        }
    }

    return s;
}

void Debugger::cpu_dump() const {
    const CPU& cpu = m_system.m_cpu;

    std::cout << std::format("pc: {:#010x}\n", cpu.m_cur_pc);
    for (usize i=0; i<CPU::NUM_REGS; i++){
        std::cout << std::format("r{}: {:#010x}\n", i, cpu.m_regs[i]);
    }
    std::cout << std::format("hi: {:#010x}\n"
            "lo: {:#010x}\n", cpu.m_hi, cpu.m_lo);
}

void Debugger::cpu_setpc(u32 pc) {
    m_system.m_cpu.set_pc(pc);
    std::cout << std::format("Set pc to {:#010x}", m_system.m_cpu.m_cur_pc) << std::endl;
}

void Debugger::mem_examine_word(u32 addr, u32 num) const {
    addr -= addr % 4;
    for (u32 i=0; i<num; i++){
        if (addr >= RAM::SIZE){
            std::cout << "Reached end of memory, stopping" << std::endl;
            break;
        }
        std::cout << std::format("{:#010x}: {:#010x}", addr, m_system.m_ram.read32(addr)) << '\n';
        addr += 4;
    }
    std::cout.flush();
}

void Debugger::mem_writefile(u32 addr, const std::string& name) {
    std::ifstream is(name, std::ios::binary);

    if (!is.is_open()){
        std::cout << std::format("File {} coudln't be opened", name) << std::endl;
        return;
    }

    is.seekg(0, is.end);
    u32 len = is.tellg();
    is.seekg(0, is.beg);

    is.read(reinterpret_cast<char*>(m_system.m_ram.m_data->data()) + addr, len);
    std::cout << std::format("Wrote {} bytes starting at {:#010x}", len, addr) << std::endl;
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
        
        try {
            if (cmd == "cpu"){
                std::string arg1;
                args >> arg1;
                if (arg1 == "dump"){
                    cpu_dump();
                } else if (arg1 == "setpc"){
                    u32 pc = read_hex(args);
                    cpu_setpc(pc);
                } else {
                    throw Debugger::ParseError("Unknown command");
                }
            } else if (cmd == "mem"){
                std::string arg1;
                args >> arg1;
                if (arg1 == "examine"){
                    u32 addr = read_hex(args);
                    u32 num = read_dec(args);
                    mem_examine_word(addr, num);
                } else if (arg1 == "writefile"){
                    u32 addr = read_hex(args);
                    std::string name = read_str(args);
                    mem_writefile(addr, name);
                } else {
                    throw Debugger::ParseError("Unknown command");
                }
            } else if (cmd == "sys"){
                std::string arg1;
                args >> arg1;
                if (arg1 == "run"){
                    sys_run();
                } else if (arg1 == "breakpoint"){
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
            std::cout << std::format("Couldn't parse command: {}", p.what()) << std::endl;
        }
    }
}

static std::atomic<bool> pending_sigint{false};

static void sigint_handler(int signal){
    pending_sigint = true;
}

void Debugger::sys_run(){
    std::cout << "Running (Ctrl-C to stop)" << std::endl;
    std::signal(SIGINT, sigint_handler);

    pending_sigint = false;
    while (!pending_sigint){
        try {
            m_system.tick();
        } catch (Panic p) {
            std::cout << std::format("System panicked: {}\nStopping", p.what()) << std::endl;
            break;
        }

        if (std::find(m_breakpoints.begin(), m_breakpoints.end(), m_system.m_cpu.m_cur_pc) != m_breakpoints.end()){
            std::cout << std::format("Reached breakpoint {:#010x}\nStopping", m_system.m_cpu.m_cur_pc) << std::endl;
            break;
        }
    }

    std::signal(SIGINT, SIG_DFL);

    pending_sigint = false;
}

void Debugger::sys_breakpoint_set(u32 addr){
    auto it = std::find(m_breakpoints.begin(), m_breakpoints.end(), addr);
    if (it != m_breakpoints.end()){
        std::cout << std::format("Breakpoint {} already exists\n", addr) << std::endl;
        return;
    }

    m_breakpoints.push_back(addr);
    return;
}

void Debugger::sys_breakpoint_remove(u32 addr){
    auto it = std::find(m_breakpoints.begin(), m_breakpoints.end(), addr);
    if (it == m_breakpoints.end()){
        std::cout << std::format("Breakpoint {} doesn't exist\n", addr) << std::endl;
        return;
    }

    m_breakpoints.erase(it);
    return;
}

void Debugger::sys_breakpoint_list(){
    for (auto x : m_breakpoints){
        std::cout << std::format("{:#010x}\n", x);
    }
    std::cout.flush();
}

};
