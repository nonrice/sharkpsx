#pragma once

#include "types.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib") 
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
#endif


namespace pse {

class Net {
public:

#ifdef _WIN32
    using socket_t = SOCKET;
    static constexpr socket_t invalid_socket = INVALID_SOCKET;
#else
    using socket_t = s32;
    static constexpr socket_t invalid_socket = -1;
#endif

    Net() = delete;

    static bool init();
    static void shutdown();
    static void close(socket_t s);

    static bool is_setup();

private:
    static bool m_setup;
};

}
