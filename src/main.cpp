#include <iostream>

#include "logging.hpp"
#include "Debugger.hpp"

int main(int argc, char** argv){
    pse::System sys;

    pse::Debugger app(sys);

    if (argc == 1){
        app.run();
    } else {
        app.run_file(std::string(argv[1]));
    }
}
