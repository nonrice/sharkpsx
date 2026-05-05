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

    virtual void write8(u32 addr, u8 val) = 0;
    virtual void write16(u32 addr, u16 val) = 0;
    virtual void write32(u32 addr, u32 val) = 0;
};

};
