#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <format>
#include <exception>

#include "types.hpp"
#include "Debugger.hpp"

namespace pse {

static void print_prompt(){
    std::cout << "> ";
}

Debugger::Debugger(System& system) : m_system(system) {
    std::cout << "PSXEMU DEBUGGER (dev)\n"
        "Enter \"help\" for help\n";
}

std::string Debugger::cpu_dump_get_str() const {
    std::stringstream out;

    const CPU& cpu = m_system.m_cpu;

    out << std::format("pc: {:#010x}\n", cpu.m_cur_pc);
    for (usize i=0; i<CPU::NUM_REGS; i++){
        out << std::format("r{}: {:#010x}\n", i, cpu.m_regs[i]);
    }
    out << std::format("hi: {:#010x}\n"
            "lo: {:#010x}\n", cpu.m_hi, cpu.m_lo);

    return out.str();
}

static std::optional<u32> read_hex(std::istream& is){
    u32 x;
    if (!(is >> std::hex >> x >> std::dec)){
        return std::nullopt;
    }
    return x;
}

static std::optional<u32> read_dec(std::istream& is){
    u32 x;
    if (!(is >> x)){
        return std::nullopt;
    }
    return x;
}

void Debugger::cpu_dump() const {
    std::cout << cpu_dump_get_str();
    std::cout.flush();
}

void Debugger::cpu_setpc(u32 pc) {
    m_system.m_cpu.set_pc(pc);
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

u32 Debugger::mem_writefile(u32 addr, const std::string& name) {
    std::ifstream is(name, std::ios::binary);

    if (!is.is_open()){
        throw std::runtime_error(std::format("File {} couldn't be opened", name));
    }

    is.seekg(0, is.end);
    u32 len = is.tellg();
    is.seekg(0, is.beg);

    if (addr + len >= RAM::SIZE){
        throw std::runtime_error("File too big");
    }

    is.read(reinterpret_cast<char*>(m_system.m_ram.m_data->data()) + addr, len);

    return len;
}

void Debugger::run(){
    while (true){
        print_prompt();

        std::string input;
        std::getline(std::cin, input);

        std::stringstream args(input);

        std::string cmd;
        args >> cmd;

        if (cmd == "cpu"){
            std::string arg1;
            args >> arg1;
            if (arg1 == "dump"){
                cpu_dump();
            } else if (arg1 == "setpc"){
                std::optional<u32> pc= read_hex(args);
                if (!pc){
                    std::cout << "Hex address required for cpu setpc" << std::endl;
                    continue;
                }
                cpu_setpc(*pc);
                std::cout << std::format("Set pc to {:#010x}", m_system.m_cpu.m_cur_pc) << std::endl;
            } else {
                std::cout << "Unknown command for cpu" << std::endl;
            }
        } else if (cmd == "mem"){
            std::string arg1;
            args >> arg1;
            if (arg1 == "examine"){
                std::optional<u32> addr = read_hex(args);
                std::optional<u32> num = read_dec(args);

                if (!addr || !num){
                    std::cout << "Expected hex addr and num for mem examine" << std::endl;
                    continue;
                }

                mem_examine_word(*addr, *num);
            } else if (arg1 == "writefile"){
                std::optional<u32> addr = read_hex(args);
                if (!addr){
                    std::cout << "Expected hex addr for mem writefile" << std::endl;
                    continue;
                }

                std::string name;
                if (!(args >> std::quoted(name))){
                    if (!(args >> name)){
                        std::cout << "Expected filename for mem writefile" << std::endl;
                        continue;
                    }
                }

                try {
                    u32 size = mem_writefile(*addr, name);
                    std::cout << std::format("Wrote {} bytes starting at {:#010x}", size, *addr) << std::endl;
                } catch (std::runtime_error& e){
                    std::cout << std::format("mem writefile: {}", e.what()) << std::endl;
                } catch (...){
                    std::cout << "Unknown error for mem writefile" << std::endl;
                }
            } else {
                std::cout << "Unkown command for mem" << std::endl;
            }
        } else {
            if (std::cin.eof()){
                break;
            }
            std::cout << "Unknown command" << std::endl;
        }
    }
}

void Debugger::system_run(){
    while (true){
        m_system.tick();
    }
}

};
