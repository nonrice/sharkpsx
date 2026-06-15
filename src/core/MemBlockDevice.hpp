#pragma once

#include <memory>

#include "Device.hpp"
#include "types.hpp"

namespace pse {

class MemBlockDevice : public Device {
public: 
    MemBlockDevice(usize size);

    u8 read8(u32 addr) override;
    u16 read16(u32 addr) override;
    u32 read32(u32 addr) override;

    void write8(u32 addr, u32 val) override;
    void write16(u32 addr, u32 val) override;
    void write32(u32 addr, u32 val) override;

    usize get_size();

protected:
    usize m_size;
    std::unique_ptr<u8[]> m_data;
};

}
