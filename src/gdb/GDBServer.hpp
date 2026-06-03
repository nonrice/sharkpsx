#pragma once

#include <span>

#include "Server.hpp"
#include "SockIO.hpp"

namespace pse {

class GDBServer : public Server {
public:
    using Server::Server;


protected:
    virtual void on_connect(Net::socket_t sock) override;

    // buf must be large enough to hold the entire packet in 
    // *transmission* format, i.e. >= configured packetsize
    ssize next_packet(std::span<char> buf, SockIO s);

private:


};

}
