#pragma once

#include <array>

#include "Device.hpp"
#include "types.hpp"

namespace pse {

class RAM : public Device {
public:
    RAM();

    u8 read8(u32 addr) override;
    u16 read16(u32 addr) override;
    u32 read32(u32 addr) override;

    void write8(u32 addr, u8 val) override;
    void write16(u32 addr, u16 val) override;
    void write32(u32 addr, u32 val) override;

    static constexpr usize SIZE_KB = 2048;
    static constexpr usize SIZE = BYTES_KB * SIZE_KB;
private:
    friend class Debugger;

    std::unique_ptr<u8[]> m_data;
};

};
