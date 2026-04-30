#pragma once

#include <concepts>
#include <type_traits>
#include <cassert>

#include "types.hpp"

namespace pse {

// it's an abstraction for bitfields
//
// i made Instr before this, so it's probably the only thing
// now that doesn't use it
//
// i will overhaul it with this sometime. imm16 casing will be a problem
// though since I had originally returned u16 for the u32.
//
// usage
// note indices 0 indexed
// union Reg {
//     u32 val;
//
//     BitField<u32, 15, 31> upper_half;
// };
//
// very simple
// Reg r1{0xFFFFFFFF};
// r1.upper_half = 0xABCD;
// assert(r1.upper_half == 0xABCD);
//
//
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
