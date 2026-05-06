#pragma once

#include <cassert>

#include "types.hpp"

namespace pse {

enum MMIOPortAccess : u8 {
    R,
    W,
    RW
};

template<RegType T, MMIOPortAccess A>
struct MMIOPort {
    T m_val;

    T read(){
        assert(A == R || A == RW);
        return m_val;
    }

    void write(u32 val_new){
        assert(A == W || A == RW);
        m_val = static_cast<T>(val_new);
    }
};

}
