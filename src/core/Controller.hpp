#pragma once

#include <atomic>

#include "SIODev.hpp"

namespace pse {

class Controller : public SIODev {
public:
    void set_cs(bool val) override;
    u8 exch(u8 val) override;
    bool get_ack() override;

    void set_switches(u16 s);

private:
    u8 m_state{}; 

    static constexpr u16 ID = 0x5A41;
    std::atomic<u16> m_switches{0xFFFF};

};

}
