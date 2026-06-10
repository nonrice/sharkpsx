#include <cassert>

#include "SigCapture.hpp"
#include "logging.hpp"

namespace pse {

volatile sig_atomic_t SigCapture::m_pending{false};
bool SigCapture::m_enabled{false};
struct sigaction SigCapture::m_old_action{};

void SigCapture::enable(){
    assert(!m_enabled);

    struct sigaction new_action{};
    new_action.sa_handler = &SigCapture::handle_sigint;
    new_action.sa_flags = 0; //no restart

    m_pending = false;
    if (sigaction(SIGINT, &new_action, &m_old_action) == -1) {
        LOG_DBG("Failed to register signal handler");
    }

    m_enabled = true;
}

void SigCapture::disable(){
    assert(m_enabled);

    if (sigaction(SIGINT, &m_old_action, nullptr) == -1) {
        LOG_DBG("Failed to unregister signal handler");
    }
    m_enabled = false;
    m_pending = false;

}

bool SigCapture::pending(){
    return m_pending;
}

void SigCapture::unset(){
    m_pending = false;
}

void SigCapture::handle_sigint([[maybe_unused]] int sig){
    m_pending = true;
}

}
