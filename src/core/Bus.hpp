#pragma once

#include <optional>
#include <iostream>

#include "Device.hpp"
#include "DummyDevice.hpp"
#include "types.hpp"

namespace pse {

class CPU;
class Device;

class Bus : public Device {
public:
    Bus(CPU* cpu, Device* ram, Device* scratch, Device* bios_rom, Device* m_redux, Device* m_gpu);

    u8 read8(u32 addr) override;
    u16 read16(u32 addr) override;
    u32 read32(u32 addr) override;

    void write8(u32 addr, u32 val) override;
    void write16(u32 addr, u32 val) override;
    void write32(u32 addr, u32 val) override;

    void set_tty(std::ostream* tty);
    void flush_tty();
    void remove_tty();
    void putchar(char ch);

    bool get_buserr();
private:
    friend class BasicDebug;
    // for debugging purposes
    std::optional<u32> m_read_addr;
    std::optional<u32> m_write_addr;
    // debugger sets them back to nullopt once processed
    // preventing multiple activations for the same read etc
    

    CPU* m_cpu;
    Device* m_ram;
    Device* m_scratch;
    Device* m_bios_rom;
    Device* m_redux;
    Device* m_gpu;
    std::ostream* m_tty;

    bool m_buserr;

    DummyDevice m_dummy;

    struct MemAccess {
        Device* dev;
        u32 addr;
    };
    MemAccess map_addr(u32 addr);
};

};
