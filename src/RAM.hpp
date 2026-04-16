#pragma once

#include <array>

#include "Device.hpp"
#include "types.hpp"

namespace pse {

class RAM : Device {
public:
    RAM();

    virtual u8 read8(u32 addr) override;
    virtual u16 read16(u32 addr) override;
    virtual u32 read32(u32 addr) override;

    virtual void write8(u32 addr, u8 val) override;
    virtual void write16(u32 addr, u16 val) override;
    virtual void write32(u32 addr, u32 val) override;

private:
    static constexpr size_t NUM_BYTES_KB = 1024;
    static constexpr size_t SIZE_KB = 2048;
    std::array<u8, SIZE_KB * NUM_BYTES_KB> m_data;

};

};
