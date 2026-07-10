#include <thread>

#include "SWRenderer.hpp"
#include "Debugger.hpp"
#include "App.hpp"
#include "Controller.hpp"

int main(int argc, char** argv){
    pse::Controller p1{};

    pse::App app(&p1);
    app.init();

    pse::SWRenderer renderer(
            [&app](auto p){ app.vram_into_buf(p); }
            );

    pse::System sys(renderer); // renderer moved!!!!
    sys.set_tty(&std::cout);
    sys.set_sio(&p1, 0);

    pse::Debugger dbg(sys);
    std::thread t(
        [argc, argv, &dbg]{
            if (argc == 1){
                dbg.run();
            } else {
                dbg.run_file(std::string(argv[1]));
            }
        }
    );

    app.run();

    t.join();
    return 0;
}
