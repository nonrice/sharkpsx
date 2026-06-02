#include <cassert>

#include "Net.hpp"
#include "Server.hpp"
#include "logging.hpp"

namespace pse {

Server::Server(u16 port) :
    m_setup(false), m_port(port) {}

Server::~Server(){
    if (m_setup){
        Net::close(m_server);
    }
}

s32 Server::init(){
    assert(Net::is_setup());
    assert(!m_setup);

    m_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_server == Net::SOCK_INVALID){
        LOG_DBG("failed to make socket");
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_server, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr))
            < 0){
        LOG_DBG("failed to bind socket");
        Net::close(m_server);
        return -1;
    }

    if (listen(m_server, 1) < 0){
        LOG_DBG("failed to listen");
        Net::close(m_server);
        return -1;
    }

    m_setup = true;
    LOG_DBG("Listening on localhost:{}", m_port);
    return 0;
}

void Server::run(){
    assert(m_setup);

    while (true){
        Net::socket_t conn = accept(m_server, NULL, NULL);
        if (conn != Net::SOCK_INVALID){
            on_connect(conn);
            Net::close(conn);
        }
    }
}

}
