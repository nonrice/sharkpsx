#pragma once

#include "Net.hpp"
#include "types.hpp"

namespace pse {

class Server {
public:
    Server(u16 port);

    s32 init();
    void shutdown();
    bool run(bool single = false);

protected:
    virtual void on_connect(Net::socket_t conn) = 0;

private:
    bool m_setup;
    u16 m_port;

    Net::socket_t m_server = Net::SOCK_INVALID;
};

}
