#include <thread>

#include "Debugger.hpp"
#include "App.hpp"

int main(int argc, char** argv){
    pse::App app{};
    app.init();

    pse::System sys{};
    sys.set_tty(&std::cout);
    sys.set_on_vblank([&app](auto p){ app.vram_into_buf(p); });
    pse::Debugger dbg{sys};

    std::jthread t(
        [argc, argv, &dbg]{
            if (argc == 1){
                dbg.run();
            } else {
                dbg.run_file(std::string(argv[1]));
            }
        }
    );

    app.run();

    return 0;
}
