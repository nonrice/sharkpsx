#pragma once

#include "MemBlockDevice.hpp"
#include "types.hpp"

namespace pse {

class RAM : public MemBlockDevice {
public:
    static constexpr usize SIZE_KB = 2048;
    static constexpr usize SIZE = BYTES_KB * SIZE_KB;

    RAM() : MemBlockDevice(SIZE) {}
private:
    friend class BasicDebug;
};

};
