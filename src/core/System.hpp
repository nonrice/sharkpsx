#pragma once

#include "CPU.hpp"
#include "Bus.hpp"
#include "RAM.hpp"
#include "Scratch.hpp"
#include "GPU.hpp"
#include "SIO.hpp"
#include "BIOSROM.hpp"
#include "ReduxDevice.hpp"
#include "Renderer.hpp"
#include "EventScheduler.hpp"

namespace pse {

class System {
public:
    System(Renderer& r);
    void tick();

    void set_tty(std::ostream* tty);
    void set_sio(SIODev* s, usize i);
    void flush_tty();
private:
    friend class BasicDebug;

    EventSched m_sched;
    CPU m_cpu{&m_bus, &m_intc};
    GPU m_gpu;
    Bus m_bus;
    RAM m_ram;
    Scratch m_scratch;
    BIOSROM m_bios_rom;
    SIO m_sio0{&m_sched, &m_intc};
    IntCtl m_intc;
    ReduxDevice m_redux{&m_bus};

};

};
