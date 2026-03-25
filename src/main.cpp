#include <iostream>
#include "logging.hpp"

#include "CPU.hpp"

int main(){
    std::cout << "Hello world\n";

    LOG("hi {}", 1);
    LOG_DBG("hi {}", 2);
}
