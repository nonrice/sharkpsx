#pragma once

#include <concepts>
#include <type_traits>
#include <cassert>

#include "types.hpp"

namespace pse {

template <typename T, usize Start, usize End>
requires(
    (std::same_as<T, u8> || std::same_as<T, u16> || std::same_as<T, u32>) &&
    (Start <= End) &&
    (End < sizeof(T) * 8)
)
struct BitField {
    T m_val;

    static constexpr usize Width = End - Start + 1;
    static constexpr T MaskBottom =
        (Width == sizeof(T) * 8) ? static_cast<T>(-1)
                                 : ((static_cast<T>(1) << Width) - 1);
    static constexpr T Mask = MaskBottom << Start;

    constexpr operator T() const {
        return (m_val & Mask) >> Start;
    }

    BitField& operator=(T val){
        assert(val <= MaskBottom);

        m_val = (m_val & (~Mask)) | (val << Start);
        return *this;
    }
};

}
