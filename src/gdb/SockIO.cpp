#include <cassert>
#include <algorithm>
#include <cstring>

#include "SockIO.hpp"
#include "logging.hpp"

namespace pse {

SockIO::SockIO(Net::socket_t sock) : 
    m_sock(sock), m_head(0), m_tail(0) {

    std::memset(m_buf, '#', BUFSZ);
}

ssize SockIO::read(std::span<char> buf){
    char* cur = buf.data();
    usize rem = buf.size();

    while (rem > 0){
        usize to_pop = std::min(BUFSZ - 1, rem);
        if (buf_fill(to_pop) < 0){
            return -1;
        }
        buf_pop({ cur, to_pop });
        cur += to_pop;
        rem -= to_pop;
    }

    return buf.size();
}

ssize SockIO::read_to(std::span<char> buf, char ch){
    char* cur = buf.data();
    usize total_read = 0;
    usize rem = buf.size();
    
    while (total_read < buf.size()){
        ssize pos = buf_count_to(ch);
        usize to_read;
        if (pos == -1){
            // safe to consume entire buffer since no ch is fuond
            to_read = buf_rem();
        } else {
            // still abort, if no more space is left!
            to_read = static_cast<usize>(pos);
        }
        to_read = std::min(to_read, rem);

        if (read({ cur, to_read }) < 0){
            return -1;
        }

        cur += to_read;
        total_read += to_read;
        rem -= to_read;

        if (pos == -1){
            // fine to hang since no more chars r there anyways
            if (buf_fill(1) < 0){
                return -1;
            }
        } else {
            return total_read;
        }
    }

    return total_read;
}

ssize SockIO::base_read(std::span<char> buf, usize cnt){
    usize rem = buf.size(); // ACTUAL remaining buffer size
    usize total_read = 0;
    char* cur = buf.data();
    // basically, we can exit once we have at least cnt
    // But it would be nice to fill the whole buf, so provided
    // there are enough chars we wouldn't get a short read usually
    while (total_read < cnt){
        ssize num_read = recv(m_sock, cur, rem, 0);
        LOG_DBG("recv called with result {}", num_read);

        if (num_read < 0){
            if (Net::get_sock_err() == Net::SOCK_ERR_EINTR){
                //retry
                continue;
            }
            LOG_DBG("read failure");
            return -1;
        }
        if (num_read == 0){
            LOG_DBG("encountered EOF while trying to read");
            return -1;
        }

        cur += num_read;
        rem -= num_read;
        total_read += num_read;
    }

    return total_read;
}

ssize SockIO::write(std::span<const char> buf){
    usize rem = buf.size();
    const char* cur = buf.data();
    while (rem > 0){
        ssize num_wrote = send(m_sock, cur, rem, 0);

        if (num_wrote < 0){
            if (Net::get_sock_err() == Net::SOCK_ERR_EINTR){
                //retry
                continue;
            }
            LOG_DBG("write failure");
            return -1;
        }

        cur += num_wrote;
        rem -= num_wrote;
    }

    return buf.size();
}

usize SockIO::buf_rem(){
    if (m_tail >= m_head){
        return m_tail - m_head;
    } else {
        return (BUFSZ - m_head) + m_tail;
    }
}

ssize SockIO::buf_count_to(char ch){
    usize i = m_head;
    ssize len = 1;
    while (i != m_tail){
        if (m_buf[i] == ch){
            return len;
        }
        i = (i+1 == BUFSZ ? 0 : i+1);
        len += 1;
    }

    return -1;
}

ssize SockIO::buf_fill(usize cnt){
    assert(cnt < BUFSZ);

    if (buf_rem() >= cnt){
        return 0;
    }

    usize total_read = 0;
    const usize rem = cnt - buf_rem();
    while (total_read < rem){
        ssize num_read;
        if (m_tail >= m_head){
            // edgecase here:
            // We typically read to end of buf. But if head is 0, we can't
            // because then tail would wrap into head ==> size=0.
            usize to_read = std::min(BUFSZ - m_tail, rem);
            num_read = base_read(
                    { m_buf + m_tail, BUFSZ - m_tail - (m_head == 0) },
                    to_read);
        } else {
            usize to_read = std::min(m_head - m_tail - 1, rem);
            num_read = base_read(
                    { m_buf + m_tail, m_head - m_tail - 1 },
                    to_read);
        }

        if (num_read < 0){
            return -1;
        }

        total_read += num_read;
        m_tail += num_read;
        if (m_tail == BUFSZ){
            m_tail = 0;
        }

    }

    return total_read;
}

void SockIO::buf_pop(std::span<char> dest){
    assert(buf_rem() >= dest.size());

    usize rem = dest.size();
    char* cur = dest.data();
    while (rem > 0){
        usize num_copy;
        if (m_head < m_tail){
            num_copy = std::min(m_tail - m_head, rem);
        } else {
            num_copy = std::min(BUFSZ - m_head, rem);
        }
        std::memcpy(cur, m_buf + m_head, num_copy);

        m_head += num_copy;
        cur += num_copy;
        rem -= num_copy;
        if (m_head == BUFSZ){
            m_head = 0;
        }
    }
}


}


