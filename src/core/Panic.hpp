#pragma once

#include <stdexcept>

namespace pse {

class Panic : public std::runtime_error {
public: 
    using std::runtime_error::runtime_error;
};

}
