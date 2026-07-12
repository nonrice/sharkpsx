#include "System.hpp"

namespace pse {

System::System(Renderer& r) :
    m_gpu(r, &m_intc),
    m_bus(
        &m_cpu,
        &m_ram,
        &m_scratch,
        &m_bios_rom,
        &m_redux,
        &m_gpu,
        &m_sio0,
        &m_intc
    )
{}

void System::set_tty(std::ostream* tty){
    m_bus.set_tty(tty);
}

void System::set_sio(SIODev* s, usize i){
    m_sio0.set_dev(s, i);
}

void System::flush_tty(){
    m_bus.flush_tty();
}

void System::tick() {
    m_cpu.tick();
    m_gpu.tick();
    m_sched.tick();
}

}
