#include <csignal>
#include <cstring>
#include <atomic>

#include "BasicDebug.hpp"
#include "logging.hpp"

namespace pse {

BasicDebug::BasicDebug(System& sys) :
    m_sys(sys), m_cpu(m_sys.m_cpu), m_bus(m_sys.m_bus) {
}

BasicDebug::RegDump BasicDebug::dump_regs(){
    return RegDump {
        .gp = m_cpu.m_regs,
        .sr = m_cpu.m_cop0.regs[CPU::COP0::SR],
        .hi = m_cpu.m_hi,
        .lo = m_cpu.m_lo,
        .bad_vaddr = m_cpu.m_cop0.regs[CPU::COP0::BADA],
        .cause = m_cpu.m_cop0.regs[CPU::COP0::CAUSE],
        .pc = m_cpu.m_cur_pc
    };
}

u8 BasicDebug::read8(u32 addr){
    return m_bus.read8(addr);
}

u16 BasicDebug::read16(u32 addr){
    return m_bus.read16(addr);
}

u32 BasicDebug::read32(u32 addr){
    return m_bus.read32(addr);
}

void BasicDebug::set_breakpoint(u32 addr){
    m_breakpts.insert(addr);
}

bool BasicDebug::remove_breakpoint(u32 addr){
    auto it = m_breakpts.find(addr);
    if (it == m_breakpts.end()){
        return false;
    }

    m_breakpts.erase(it);
    return true;
}

void BasicDebug::set_watchpoint_read(u32 addr){
    m_read_watchpts.insert(addr);
}

bool BasicDebug::remove_watchpoint_read(u32 addr){
    auto it = m_read_watchpts.find(addr);
    if (it == m_read_watchpts.end()){
        return false;
    }

    m_read_watchpts.erase(it);
    return true;
}

void BasicDebug::set_watchpoint_write(u32 addr){
    m_write_watchpts.insert(addr);
}

bool BasicDebug::remove_watchpoint_write(u32 addr){
    auto it = m_write_watchpts.find(addr);
    if (it == m_write_watchpts.end()){
        return false;
    }

    m_write_watchpts.erase(it);
    return true;
}

void BasicDebug::step(){
    // burning a halt cycle is NOT a step
    while (m_cpu.m_rem_halt != 0){
        m_sys.tick();
    }

    if (m_sys.m_cpu.m_cur_pc == 0x80030000){
        if (m_sideload_data){
            sideload();
        }
    }

    m_sys.tick();
}

u32 BasicDebug::read_sideld(usize i){
    assert(m_sideload_data);

    return (*m_sideload_data)[i] |
        ((*m_sideload_data)[i+1] << 8) | 
        ((*m_sideload_data)[i+2] << 16) | 
        ((*m_sideload_data)[i+3] << 24);
}

static std::atomic<bool> pending_sigint;
static void sigint_handler([[maybe_unused]] int signal){
    pending_sigint = true;
}

BasicDebug::StopReason BasicDebug::cont(){
    pending_sigint = false;

    std::signal(SIGINT, sigint_handler);
    // some mem ops might have set these
    m_bus.m_read_addr = std::nullopt;
    m_bus.m_write_addr = std::nullopt;
    while (!pending_sigint){
        try {
            step();
        } catch (Panic p){
            std::signal(SIGINT, SIG_DFL);
            return StopReason {
                .reason = StopReason::PANIC,
                .msg = p.what()
            };
        }

        if (m_breakpts.find(m_cpu.m_cur_pc) != m_breakpts.end()){
            std::signal(SIGINT, SIG_DFL);
            return StopReason {.reason = StopReason::BREAKPOINT };
        }

        if (m_bus.m_read_addr){
            u32 last_read = *m_bus.m_read_addr;
            m_bus.m_read_addr = std::nullopt;
            if (m_read_watchpts.find(last_read) != m_read_watchpts.end()){
                std::signal(SIGINT, SIG_DFL);
                return StopReason {.reason = StopReason::WATCHPOINT_WRITE, .addr = last_read };
            }
        }

        if (m_bus.m_write_addr){
            u32 last_write = *m_bus.m_write_addr;
            m_bus.m_write_addr = std::nullopt;
            if (m_write_watchpts.find(last_write) != m_write_watchpts.end()){
                std::signal(SIGINT, SIG_DFL);
                return StopReason {.reason = StopReason::WATCHPOINT_WRITE, .addr = last_write };
            }
        }
    }
    std::signal(SIGINT, SIG_DFL);

    pending_sigint = false;
    return StopReason {.reason = StopReason::INTERRUPT};
}

void BasicDebug::write8(u32 addr, u8 val){
    m_bus.write8(addr, val);
}

void BasicDebug::write16(u32 addr, u16 val){
    m_bus.write16(addr, val);
}

void BasicDebug::write32(u32 addr, u32 val){
    m_bus.write32(addr, val);
}

void BasicDebug::set_pc(u32 addr){
    m_cpu.set_pc(addr);
}

void BasicDebug::remove_sideload(){
    m_sideload_data = std::nullopt;
}

void BasicDebug::set_sideload(std::vector<u8> data){
    m_sideload_data = std::move(data);

    if (read_sideld(0) == 0x582d5350
            && read_sideld(4) == 0x45584520){
        return;
    }

    throw std::runtime_error("attempting to set sideload of malformed ps-exe");
}

void BasicDebug::set_biosrom(std::vector<u8> data){
    std::memcpy(m_sys.m_bios_rom.m_data.get(), data.data(), 
            sizeof(u8) * std::min(data.size(), BIOSROM::SIZE));
}

void BasicDebug::sideload(){
    if (!m_sideload_data){
        throw std::runtime_error("no executable provided to sideload");
    }

    u32 start_pc = read_sideld(0x10);
    u32 dest_addr = read_sideld(0x18);
    u32 len = read_sideld(0x1c);
    u32 sp = read_sideld(0x30);


    LOG_DBG("sharkpsx sideload: Successfully parsed PSEXE");
    LOG_DBG("  start_pc: " HEX32, start_pc);
    LOG_DBG("  dest_addr: " HEX32, dest_addr);
    LOG_DBG("  len: {} bytes", len);
    LOG_DBG("  sp: " HEX32, sp);

    dest_addr &= 0x1FFFFFFF;
    std::memcpy(dest_addr + m_sys.m_ram.m_data.get(), 0x800 + m_sideload_data->data(), 
            sizeof(u8) * len);
    m_sys.m_cpu.set_pc(start_pc);
    if (sp != 0){
        m_sys.m_cpu.m_regs[CPU::SP] = sp;
    }
    m_sys.m_cpu.m_regs[CPU::RA] = 0xFFFFFFFF;// openbios crashes after shell returns anyways so...
    LOG_DBG("sharkpsx sideload: Loaded {} bytes", len);

    remove_sideload();
}



};
