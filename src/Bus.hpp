#pragma once

#include "types.hpp"

namespace pse {

class Bus {
public:
    void read8(u32 addr);
    void read16(u32 addr);
    void read32(u32 addr);
    void write8(u32 addr, u8 val);
    void write16(u32 addr, u16 val);
    void write32(u32 addr, u32 val);

private:
};

};
