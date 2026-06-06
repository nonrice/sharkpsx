#pragma once

#include "BasicDebug.hpp"
#include "GDBServer.hpp"

namespace pse {

class PSXServer : public GDBServer {
public:
    PSXServer(BasicDebug& d, u16 port);

protected:
    virtual void handle_rsp(RSPHandler& h) override;

private:
    BasicDebug& m_dbg;

};

}
