#include <cassert>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "SigCapture.hpp"
#include "Server.hpp"
#include "logging.hpp"

namespace pse {

Server::Server(u16 port) :
    m_setup(false), m_port(port) {}

void Server::shutdown(){
    if (m_setup){
        close(m_server);
        m_setup = false;
    }
}

s32 Server::init(){
    assert(!m_setup);

    m_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_server < 0){
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
        close(m_server);
        return -1;
    }

    if (listen(m_server, 1) < 0){
        LOG_DBG("failed to listen");
        close(m_server);
        return -1;
    }

    m_setup = true;
    LOG_DBG("Listening on localhost:{}", m_port);
    return 0;
}

bool Server::run(){
    if (!m_setup){
        return false;
    }
    
    SigCapture::enable();
    while (!SigCapture::pending()){
        int conn = accept(m_server, NULL, NULL);
        if (conn > 0){
            on_connect(conn);
            close(conn);
        }
    }
    SigCapture::disable();

    return true;
}

}
