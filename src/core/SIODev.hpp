#pragma once

#include "types.hpp"

namespace pse {

class SIODev {
public:
    ~SIODev() = default;

    virtual void set_cs(bool val) = 0;
    virtual u8 exch(u8 val) = 0;
    virtual bool get_ack() = 0;

};

}
