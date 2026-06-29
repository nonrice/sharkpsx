#pragma once

#include "MemBlockDevice.hpp"
#include "types.hpp"

namespace pse {

class Scratch : public MemBlockDevice {
public:
    static constexpr usize SIZE_KB = 1;
    static constexpr usize SIZE = BYTES_KB * SIZE_KB;

    Scratch() : MemBlockDevice(SIZE) {}
private:
    friend class BasicDebug;
};

};
