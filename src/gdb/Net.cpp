#include <cassert>

#include "Net.hpp"

namespace pse {

bool Net::m_setup = false;

bool Net::init() {
    assert(!m_setup);

#ifdef _WIN32
    WSADATA wsaData;
    // Winsock ver 2.2
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
#endif

    m_setup = true;
    return true;
}

void Net::shutdown() {
    assert(m_setup);
#ifdef _WIN32
    WSACleanup();
#endif

    m_setup = false;
}

void Net::close(socket_t s) {
    assert(m_setup);

#ifdef _WIN32
    closesocket(s);
#else
    ::close(s); //avoid recursion!
#endif
}

bool Net::is_setup(){
    return m_setup;
}

}

