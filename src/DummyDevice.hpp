#pragma once

#include "Device.hpp"

namespace pse {

// this exists so in the bus i can move on
// with writes to unimplmented devices
class DummyDevice : public Device {
public:
    DummyDevice() {}

    inline u8 read8([[maybe_unused]] u32 addr) override {
        return 67;
    }

    inline u16 read16([[maybe_unused]] u32 addr) override {
        return 67;
    }

    inline u32 read32([[maybe_unused]] u32 addr) override {
        return 67;
    }

    inline void write8(
            [[maybe_unused]] u32 addr, 
            [[maybe_unused]] u8 val
            ) override {
        return;
    }

    inline void write16(
            [[maybe_unused]] u32 addr,
            [[maybe_unused]] u16 val
        ) override {
        return;
    }

    inline void write32(
            [[maybe_unused]] u32 addr,
            [[maybe_unused]] u32 val
            ) override {
        return;
    }
};

}
