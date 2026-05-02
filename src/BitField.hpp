#pragma once

#include <concepts>
#include <type_traits>
#include <cassert>
#include <format>

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
template <RegType T, usize Start, usize End>
requires(
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

template <usize Start, usize End>
using bf32 = BitField<u32, Start, End>;

template <usize Start, usize End>
using bf16 = BitField<u16, Start, End>;

template <usize Start, usize End>
using bf8 = BitField<u8, Start, End>;

}

// std::format extension
// Necessary for debugger...
namespace std {
// i dont know why this is necessary
// gemini told me
// supposedly clang has a bug in libcpp for format?? :skull:
template <pse::RegType T, pse::usize Start, pse::usize End>
struct formatter<pse::BitField<T, Start, End>> {
    
    std::formatter<T> underlying;

    constexpr auto parse(std::format_parse_context& ctx) {
        return underlying.parse(ctx);
    }

    template <typename FormatContext>
    auto format(const pse::BitField<T, Start, End>& bf, FormatContext& ctx) const {
        return underlying.format(static_cast<T>(bf), ctx);
    }
};

}


