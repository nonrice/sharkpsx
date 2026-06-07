#pragma once

#include <string_view>
#include <span>

#include "Net.hpp"

namespace pse {

class SockIO {
public:
    SockIO(Net::socket_t sock);

    // completely fill buf, possibly block. Buffered
    // Return bytes read/-1
    ssize read(std::span<char> buf);
    // write buf. unbuffered
    // return bytes wrote/-1
    ssize write(std::string_view buf);
    // Fill buf until ch is reached, including ch, or
    // until buf is full
    // return bytes wrote/-1
    ssize read_to(std::span<char> buf, char ch);

private:
    Net::socket_t m_sock;

    static constexpr usize BUFSZ = 4096;
    char m_buf[BUFSZ]{};
    // convention: head=tail ==> buffer is empty
    // head poitns to next valid char
    // tail points to where to write the next read char
    usize m_head;
    usize m_tail;

    usize buf_rem();
    // return -1 if not found
    ssize buf_count_to(char ch);
    void buf_pop(std::span<char> dest);
    // reads such that buffer has at least cnt chars
    // Return: num read, -1 on fail
    ssize buf_fill(usize cnt);

    // Fill a range [l, r) with chars, guarantee to have cnt chars
    // Intended for filling buf
    // Return: num read, -1 on fail
    ssize base_read(std::span<char> buf, usize cnt);
};

}
