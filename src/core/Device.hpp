#pragma once

#include "types.hpp"

namespace pse {

class Bus;

class Device {
public:
    virtual ~Device() = default;

    virtual u8 read8(u32 addr) = 0;
    virtual u16 read16(u32 addr) = 0;
    virtual u32 read32(u32 addr) = 0;

    // why 32 for w8 and w16?
    // it's because on the ps, the bus is actually 32 bit only
    //
    // then, enables are used to choose which bytes r actually used
    // but it's necessary to preserve the entire 32 bit gpr since
    // writing to mmio regs actually ignores the byte enables
    //
    // On the other hand, stuff like main ram does respect these enables
    virtual void write8(u32 addr, u32 val) = 0;
    virtual void write16(u32 addr, u32 val) = 0;
    virtual void write32(u32 addr, u32 val) = 0;
};

};
