#pragma once

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    
    #pragma comment(lib, "ws2_32.lib") 

    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    
    typedef int socket_t;
    const int INVALID_SOCKET = -1;
    const int SOCKET_ERROR = -1;
#endif

namespace pse {

namespace Network {
    bool init();
    void shutdown();
    void socket_close();
    
};

}
