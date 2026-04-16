#include <iostream>
#include "logging.hpp"

#include "Debugger.hpp"

int main(){
    pse::System sys;

    pse::Debugger app(sys);

    app.run();
}
