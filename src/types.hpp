#pragma once

#include <cstdint>
#include <cstddef>
#include <concepts>

namespace pse {
using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8  = std::uint8_t;

using s64 = std::int64_t;
using s32 = std::int32_t;
using s16 = std::int16_t;
using s8  = std::int8_t;

using usize = std::size_t;
using ssize = std::ptrdiff_t;

constexpr usize BYTES_KB = 1024;

template <typename T>
concept RegType = 
            std::same_as<T, u8> || std::same_as<T, u16> || std::same_as<T, u32>;
}
