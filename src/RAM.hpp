#pragma once

#include <array>

#include "Device.hpp"
#include "types.hpp"

namespace pse {

class RAM : public Device {
public:
    RAM();

    virtual u8 read8(u32 addr) override;
    virtual u16 read16(u32 addr) override;
    virtual u32 read32(u32 addr) override;

    virtual void write8(u32 addr, u8 val) override;
    virtual void write16(u32 addr, u16 val) override;
    virtual void write32(u32 addr, u32 val) override;

    static constexpr usize NUM_BYTES_KB = 1024;
    static constexpr usize SIZE_KB = 2048;
    static constexpr usize SIZE = NUM_BYTES_KB * SIZE_KB;
private:
    friend class Debugger;

    std::unique_ptr<std::array<u8, SIZE>> m_data;

};

};
