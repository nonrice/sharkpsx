#pragma once

#include "types.hpp"

namespace pse {

class Server {
public:
    Server(u16 port);

    s32 init();
    void shutdown();
    bool run();

protected:
    virtual void on_connect(int conn) = 0;

private:
    bool m_setup;
    u16 m_port;

    int m_server = -1;
};

}
