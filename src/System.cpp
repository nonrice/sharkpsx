#include "System.hpp"

namespace pse {

System::System() : m_cpu(&m_bus), m_bus(&m_cpu, &m_ram, &m_bios_rom) {
}

void System::tick() {
    m_cpu.tick();
}

}
