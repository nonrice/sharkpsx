#pragma once

#include <string>
#include <exception>

namespace pse {

class Panic : public std::runtime_error {
public: 
    using std::runtime_error::runtime_error;
};

}
