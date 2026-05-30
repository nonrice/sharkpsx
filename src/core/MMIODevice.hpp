#pragma once

#include "Device.hpp"
#include "types.hpp"

namespace pse {

class MMIODevice : public Device {
protected:
    virtual u32 read(u32 offset) = 0;
    virtual void write(u32 offset, u32 val) = 0;

public:
    u8 read8(u32 addr) override {
        return read(addr);
    }

    u16 read16(u32 addr) override {
        return read(addr);
    }

    u32 read32(u32 addr) override {
        return read(addr);
    }

    void write8(u32 addr, u32 val) override {
        write(addr, val);
    }

    void write16(u32 addr, u32 val) override {
        write(addr, val);
    }

    void write32(u32 addr, u32 val) override {
        write(addr, val);
    }
};

}
