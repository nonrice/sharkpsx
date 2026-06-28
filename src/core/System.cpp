#include "System.hpp"

namespace pse {

System::System(Renderer& r) :
    m_cpu(&m_bus),
    m_gpu(r),
    m_bus(
        &m_cpu, &m_ram, &m_bios_rom, &m_redux, &m_gpu)
{

}

void System::set_tty(std::ostream* tty){
    m_bus.set_tty(tty);
}

void System::flush_tty(){
    m_bus.flush_tty();
}

void System::tick() {
    m_cpu.tick();
    m_gpu.tick();
}

}
