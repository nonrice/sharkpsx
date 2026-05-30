#pragma once

#include "CPU.hpp"
#include "Bus.hpp"
#include "RAM.hpp"
#include "BIOSROM.hpp"
#include "ReduxDevice.hpp"

namespace pse {

class System {
public:
    System();

    void tick();

private:
    friend class BasicDebug;

    CPU m_cpu;
    Bus m_bus;
    RAM m_ram;
    BIOSROM m_bios_rom;
    ReduxDevice m_redux;

};

};
