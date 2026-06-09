#include "Debugger.hpp"

int main(int argc, char** argv){
    pse::System sys{};
    sys.set_tty(&std::cout);

    pse::Debugger app(sys);

    if (argc == 1){
        app.run();
    } else {
        app.run_file(std::string(argv[1]));
    }
}
