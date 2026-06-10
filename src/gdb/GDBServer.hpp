#pragma once

#include <string_view>
#include <optional>

#include "Server.hpp"
#include "SockIO.hpp"

namespace pse {

class GDBServer : public Server {
public:
    using Server::Server;


protected:
    virtual void on_connect(int sock) override;

    class RSPHandler {
    public:
        static constexpr usize PACK_SIZE = 0x1000;

        RSPHandler(int conn);

        void set_ack(bool ack);

        // buf must be large enough to hold the entire packet in 
        // *transmission* format, i.e. >= configured packetsize
        std::optional<std::string_view> next();

        // This will escape things, so potentially larger
        ssize write(std::string_view buf);

        bool pending_int();

    private:
        SockIO m_s;
        bool m_ack;

        char m_buf[PACK_SIZE];
    };

    virtual void handle_rsp(RSPHandler& h) = 0;

private:


};

}
