#pragma once

#include <format>
#include <iostream>

#define HEX32 "{:#010x}"
#define HEX16 "{:#06x}"
#define HEX8 "{:#03x}"

#ifdef NDEBUG
#define LOG_DBG(...) \

#define LOG(msg, ...) \
    std::clog << std::vformat(msg, std::make_format_args(__VA_ARGS__)) << '\n';
#else
#define LOG_DBG(msg, ...) \
    std::cerr << std::vformat(msg, std::make_format_args(__VA_ARGS__)) << std::endl;

#define LOG(msg, ...) \
    std::cerr << std::vformat(msg, std::make_format_args(__VA_ARGS__)) << std::endl;
#endif
