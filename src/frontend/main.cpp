#include <thread>

#include "SWRenderer.hpp"
#include "Debugger.hpp"
#include "App.hpp"

int main(int argc, char** argv){
    pse::App app{};
    app.init();

    pse::SWRenderer renderer(
            [&app](auto p){ app.vram_into_buf(p); }
            );

    pse::System sys(renderer); // renderer moved!!!!
    sys.set_tty(&std::cout);

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
