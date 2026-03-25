#pragma once

#include <format>
#include <iostream>

#ifdef NDEBUG
#define LOG_DBG(...) \

#define LOG(msg) \
    std::clog << std::format(msg, ##__VA_ARGS__) << '\n';
#else
#define LOG_DBG(msg, ...) \
    std::cerr << std::format(msg, ##__VA_ARGS__) << std::endl;

#define LOG(msg, ...) \
    std::cerr << std::format(msg, ##__VA_ARGS__) << std::endl;
#endif
