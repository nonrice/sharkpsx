#include "Network.hpp"

namespace pse {

namespace Network {

bool init() {
#ifdef _WIN32
    WSADATA wsaData;
    // Winsock ver 2.2
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
#endif
    return true;
}

void shutdown() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void socket_close(socket_t s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

}

}
