#pragma once

#include <csignal>

namespace pse {

class SigCapture {
public:
    static void enable();
    static void disable();
    static bool pending();

private:
    static volatile sig_atomic_t m_pending;
    static bool m_enabled;
    static struct sigaction m_old_action;

    static void handle_sigint(int sig);
};

}
