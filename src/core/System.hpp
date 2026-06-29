#pragma once

#include "CPU.hpp"
#include "Bus.hpp"
#include "RAM.hpp"
#include "Scratch.hpp"
#include "GPU.hpp"
#include "BIOSROM.hpp"
#include "ReduxDevice.hpp"
#include "Renderer.hpp"

namespace pse {

class System {
public:
    System(Renderer& r);
    void tick();

    void set_tty(std::ostream* tty);
    void flush_tty();
private:
    friend class BasicDebug;

    CPU m_cpu;
    GPU m_gpu;
    Bus m_bus;
    RAM m_ram;
    Scratch m_scratch;
    BIOSROM m_bios_rom;
    ReduxDevice m_redux{&m_bus};

};

};
