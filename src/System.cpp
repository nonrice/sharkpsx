#include "System.hpp"

namespace pse {

System::System() : m_bus(&m_cpu, &m_ram) {
}

void System::tick() {
    m_cpu.tick();
}

}
