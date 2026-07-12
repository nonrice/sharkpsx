#pragma once

#include <atomic>

#include "SIODev.hpp"

namespace pse {

class Controller : public SIODev {
public:
    void set_cs(bool val) override;
    u8 exch(u8 val) override;
    bool get_ack() override;

    enum Switch : u16 {
        SEL = 1 << 0,
        L3 = 1 << 1,
        R3 = 1 << 2,
        START = 1 << 3,
        UP = 1 << 4,
        RIGHT = 1 << 5,
        DOWN = 1 << 6, 
        LEFT = 1 << 7,
        L2 = 1 << 8,
        R2 = 1 << 9, 
        L1 = 1 << 10, 
        R1 = 1 << 11, 
        TRI = 1 << 12, 
        CIR = 1 << 13, 
        X = 1 << 14, 
        SQR = 1 << 15,
    };

    void set_switches(u16 s);

private:
    u8 m_state{}; 

    static constexpr u16 ID = 0x5A41;
    std::atomic<u16> m_switches{0xFFFF};

};

}
