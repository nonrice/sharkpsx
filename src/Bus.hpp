#pragma once

#include "Device.hpp"
#include "types.hpp"

namespace pse {

class CPU;
class Device;

class Bus {
public:
    Bus(CPU* cpu, Device* ram);

    u8 read8(u32 addr);
    u16 read16(u32 addr);
    u32 read32(u32 addr);

    void write8(u32 addr, u8 val);
    void write16(u32 addr, u16 val);
    void write32(u32 addr, u32 val);
private:
    CPU* m_cpu;
    Device* m_ram;

    struct MemAccess {
        Device* dev;
        u32 addr;
    };
    MemAccess map_addr(u32 addr);
};

};
