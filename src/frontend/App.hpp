#pragma once

#include <memory>

namespace pse {

class App {
public:
    App();
    ~App();

    bool init();
    void run();

private:
    struct Impl;

    // so we can hide SDL from core
    std::unique_ptr<Impl> m_imp;

};

};
