#pragma once

#include <span>
#include <limits>
#include <cassert>
#include <string_view>

#include "logging.hpp"
#include "types.hpp"


namespace pse {

constexpr usize UMAX = std::numeric_limits<usize>::max();

// byte conversion
u64 b2i(char ch);
char i2b(u64 i);

// string conversion
// Base up to 16
u64 s2i(std::string_view s, u64 b = 16);

ssize i2sl(std::span<char> s, u64 x, u64 b = 16);
ssize i2sr(std::span<char> s, u64 x, u64 b = 16);

// return -1 if not found
ssize find(std::string_view s, char ch);

template <usize sz>
class StrBuilder {
public:
    std::string_view to_span() const;

    ssize push(std::string_view s);
    ssize push_int(u64 x, u64 b=16);
    ssize push_int_pad(u64 x, usize w, u64 b=16);

private:
    const usize m_bufsz = sz;
    usize m_sz = 0;
    char m_buf[sz];
    std::span<char> m_buf_span = m_buf;
};

template <usize sz>
std::string_view StrBuilder<sz>::to_span() const {
    return { m_buf, m_sz };
}

template <usize sz>
ssize StrBuilder<sz>::push(std::string_view s) {
    if (s.size() + m_sz > m_bufsz){
        LOG_DBG("string too big to append");
        return -1;
    }

    std::ranges::copy(s, m_buf_span.subspan(m_sz).begin());
    m_sz += s.size();

    return m_sz;
}

template <usize sz>
ssize StrBuilder<sz>::push_int(u64 x, u64 b){
    ssize len = i2sl(m_buf_span.subspan(m_sz), x, b);
    if (len < 0){
        return -1;
    }
    m_sz += len;
    return m_sz;
}

template <usize sz>
ssize StrBuilder<sz>::push_int_pad(u64 x, usize w, u64 b){
    ssize len = i2sr(m_buf_span.subspan(m_sz, w), x, b);
    if (len < 0){
        return -1;
    }
    m_sz += w;
    return m_sz;
}


};
