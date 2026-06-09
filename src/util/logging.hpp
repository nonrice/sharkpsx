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

template <typename... Args>
inline void _log_dbg_impl(std::string_view msg, Args&&... args) {
    std::cerr << std::vformat(msg, std::make_format_args(args...)) << std::endl;
}

    #define LOG_DBG(msg, ...) \
        _log_dbg_impl(msg __VA_OPT__(,) __VA_ARGS__)
#endif

