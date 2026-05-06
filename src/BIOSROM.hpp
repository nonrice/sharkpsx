#pragma once

#include <array>

#include "MemBlockDevice.hpp"
#include "types.hpp"
#include "Panic.hpp"

namespace pse {

class BIOSROM : public MemBlockDevice {
public:
    static constexpr usize SIZE_KB = 4096;
    static constexpr usize SIZE = BYTES_KB * SIZE_KB;

    BIOSROM() : MemBlockDevice(SIZE) {};

    void write8([[maybe_unused]] u32 addr, [[maybe_unused]] u32 val) override
    {
        throw Panic("Trying to write to BIOS ROM");
    }

    void write16([[maybe_unused]] u32 addr, [[maybe_unused]] u32 val) override
    {
        throw Panic("Trying to write to BIOS ROM");
    }

    void write32([[maybe_unused]] u32 addr, [[maybe_unused]] u32 val) override
    {
        throw Panic("Trying to write to BIOS ROM");
    }

private:
    friend class Debugger;
};

};
