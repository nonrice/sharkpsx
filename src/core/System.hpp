#pragma once

#include "CPU.hpp"
#include "Bus.hpp"
#include "RAM.hpp"
#include "GPU.hpp"
#include "BIOSROM.hpp"
#include "ReduxDevice.hpp"

namespace pse {

class System {
public:
    System();
    void tick();

    void set_tty(std::ostream* tty);
    void flush_tty();
    void remove_tty();
private:
    friend class BasicDebug;

    CPU m_cpu;
    Bus m_bus;
    RAM m_ram;
    GPU m_gpu;
    BIOSROM m_bios_rom;
    ReduxDevice m_redux;

};

};
