#pragma once

#include <cassert>
#include <format>

#include "types.hpp"

namespace pse {

// it's an abstraction for bitfields because builtin bitfield=bad
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
        return get();
    }

    constexpr T get() const {
        return (m_val & Mask) >> Start;
    }

    BitField& operator=(T val){
        // assert(val <= MaskBottom);
        val &= MaskBottom;

        m_val = (m_val & (~Mask)) | (val << Start);
        return *this;
    }
};

template <usize Start, usize End>
using bf32 = BitField<u32, Start, End>;

template <usize Pos>
using b32 = BitField<u32, Pos, Pos>;

template <usize Start, usize End>
using bf16 = BitField<u16, Start, End>;

template <usize Start, usize End>
using bf8 = BitField<u8, Start, End>;

template <RegType T, usize Start, usize End>
T get_bf(T x){
    return x >> Start << Start << End >> End;
}

template <usize Start, usize End>
constexpr auto& get_bf32 = get_bf<u32, Start, End>;

template <usize Pos>
constexpr auto& get_b32 = get_bf<u32, Pos, Pos>;

// conveniences!
union Pack16_32 {
    u32 val;
    bf32<0, 15> lo;
    bf32<16, 31> hi;
};

union Pack8_32 {
    u32 val;
    bf32<0, 7> a;
    bf32<8, 15> b;
    bf32<16, 23> c;
    bf32<24, 31> d;
};



}

// std::format extension
// Necessary for debugger...
namespace std {
// i dont know why this is necessary
// 
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


