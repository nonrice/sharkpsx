#pragma once

#include <format>
#include <iostream>

#define HEX32 "{:#010x}"
#define HEX16 "{:#06x}"
#define HEX8 "{:#03x}"

#include <iostream>
#include <format>
#include <type_traits> // Required for std::is_constant_evaluated

#ifdef NDEBUG
    #define LOG_DBG(...) do {} while(0)

    #define LOG(msg, ...) \
        do { \
            if (!std::is_constant_evaluated()) { \
                std::clog << std::vformat(msg, std::make_format_args(__VA_ARGS__)) << '\n'; \
            } \
        } while(0)
#else
    #define LOG_DBG(msg, ...) \
        do { \
            if (!std::is_constant_evaluated()) { \
                std::cerr << std::vformat(msg, std::make_format_args(__VA_ARGS__)) << std::endl; \
            } \
        } while(0)

    #define LOG(msg, ...) \
        do { \
            if (!std::is_constant_evaluated()) { \
                std::cerr << std::vformat(msg, std::make_format_args(__VA_ARGS__)) << std::endl; \
            } \
        } while(0)
#endif
