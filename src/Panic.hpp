#pragma once

#include <string>
#include <exception>

namespace pse {

class Panic : public std::exception {
public: 
    inline explicit Panic(std::string msg) : m_msg(std::move(msg)) {}

    inline const char* what() const noexcept override {
        return m_msg.c_str();
    }
private:
    std::string m_msg;
};

}
