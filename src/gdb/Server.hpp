#pragma once

#include "Net.hpp"
#include "types.hpp"

namespace pse {

class Server {
public:
    virtual ~Server();
    Server(u16 port);

    s32 init();
    void run();

protected:
    virtual void on_connect(Net::socket_t conn) = 0;

private:
    bool m_setup = false;
    u16 m_port;

    Net::socket_t m_server = Net::invalid_socket;
};

}
