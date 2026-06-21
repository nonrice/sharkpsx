#pragma once


#define HEX64 "{:#018x}"
#define HEX32 "{:#010x}"
#define HEX16 "{:#06x}"
#define HEX8 "{:#03x}"

#ifdef NDEBUG
    #define LOG_DBG(...) do {} while(0)
#else
#include <format>
#include <iostream>
#include <source_location>

constexpr std::string_view get_basename(std::string_view path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string_view::npos) ? path : path.substr(pos + 1);
}

template <typename... Args>
inline void _log_dbg_impl(const std::source_location& loc, std::string_view msg, Args&&... args) {
    // Print the file and line number prefix, followed by the formatted message
    std::cerr << std::format("[{}:{}] ", get_basename(loc.file_name()), loc.line())
              << std::vformat(msg, std::make_format_args(args...)) 
              << '\n'; // Using '\n' is generally faster than std::endl
}

#define LOG_DBG(msg, ...) \
    _log_dbg_impl(std::source_location::current(), msg __VA_OPT__(,) __VA_ARGS__)
#endif

// template <typename... Args>
// inline void _log_dbg_impl(std::string_view msg, Args&&... args) {
//     std::cerr << std::vformat(msg, std::make_format_args(args...)) << std::endl;
// }

//     #define LOG_DBG(msg, ...) \
//         _log_dbg_impl(msg __VA_OPT__(,) __VA_ARGS__)
// #endif

