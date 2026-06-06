#pragma once

#include <unordered_set>
#include <array>
#include <vector>

#include "System.hpp"
#include "types.hpp"

namespace pse {

class BasicDebug {
public:
    BasicDebug(System& sys);

    struct RegDump {
        std::array<u32, 32> gp;
        u32 sr;
        u32 hi, lo;
        u32 bad_vaddr;
        u32 cause;
        u32 pc;
    };
    RegDump dump_regs();

    u8 read8(u32 addr);
    u16 read16(u32 addr);
    u32 read32(u32 addr);
    void set_breakpoint(u32 addr);
    // convention is return false on failure
    bool remove_breakpoint(u32 addr);
    void set_watchpoint_read(u32 addr);
    bool remove_watchpoint_read(u32 addr);
    void set_watchpoint_write(u32 addr);
    bool remove_watchpoint_write(u32 addr);
    void step();

    struct StopPanic { std::string msg; };
    struct StopWatchpointRead { u32 addr; };
    struct StopWatchpointWrite { u32 addr; };
    struct StopBreakpoint {};
    struct StopInterrupt {};
    struct StopReason {
        enum Reason {
            PANIC,
            WATCHPOINT_READ,
            WATCHPOINT_WRITE,
            BREAKPOINT,
            INTERRUPT,
        };

        Reason reason;
        u32 addr;
        std::string msg;
    };
    StopReason cont(std::atomic<bool>& sigint);

    void write8(u32 addr, u8 val);
    void write16(u32 addr, u16 val);
    void write32(u32 addr, u32 val);

    void set_pc(u32 addr);

    void set_sideload(std::vector<u8> data);
    void remove_sideload();
    void set_biosrom(std::vector<u8> data);

private:
    System& m_sys;

    // pretty much just aliases for sys
    CPU& m_cpu;
    Bus& m_bus;

    std::optional<std::vector<u8>> m_sideload_data;
    u32 read_sideld(usize i);

    void sideload();

    void tick();
    
    std::unordered_set<u32> m_read_watchpts;
    std::unordered_set<u32> m_write_watchpts;
    std::unordered_set<u32> m_breakpts;
};

}
