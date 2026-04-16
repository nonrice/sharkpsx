#include <iostream>
#include <string>
#include <sstream>

#include "Debugger.hpp"

namespace pse {

static void print_prompt(){
    std::cout << "> ";
}

Debugger::Debugger(System& system) : m_system(system) {}

void Debugger::run(){
    while (true){
        print_prompt();

        std::string input;
        std::getline(std::cin, input);

        std::stringstream args(input);

        std::string cmd;
        args >> cmd;

        if (cmd == "help"){
            std::printf(
                    "Commands:\n"
                    "help: Show help\n"
                );
        } else {
            std::printf("Unknown command\n");
        }
    }
}

};
