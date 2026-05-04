#pragma once

#include <optional>

#include "Device.hpp"
#include "DummyDevice.hpp"
#include "types.hpp"

namespace pse {

class CPU;
class Device;

class Bus {
public:
    Bus(CPU* cpu, Device* ram, Device* bios_rom);

    u8 read8(u32 addr);
    u16 read16(u32 addr);
    u32 read32(u32 addr);

    void write8(u32 addr, u8 val);
    void write16(u32 addr, u16 val);
    void write32(u32 addr, u32 val);
private:
    friend class Debugger;
    // for debugging purposes
    std::optional<u32> m_read_addr;
    std::optional<u32> m_write_addr;
    // debugger sets them back to nullopt once processed
    // preventing multiple activations for the same read etc

    CPU* m_cpu;
    Device* m_ram;
    Device* m_bios_rom;

    DummyDevice m_dummy;

    struct MemAccess {
        Device* dev;
        u32 addr;
    };
    MemAccess map_addr(u32 addr);
};

};
