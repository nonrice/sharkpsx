#pragma once

#include <array>

#include "types.hpp"

namespace pse {

template<typename T, usize C>
class Fifo {
public:
    const usize MAX_SIZE = C;

    usize size(){
        if (m_b >= m_f){
            return m_b - m_f;
        }
        return m_f + (C+1 - m_b - 1);
    }

    bool full(){
        return size() == MAX_SIZE;
    }

    bool empty(){
        return size() == 0;
    }

    void push(T x){
        assert(!full());
        m_data[m_b] = x;
        m_b = (m_b + 1) % (C + 1);
    }

    T pop(){
        assert(!empty());
        T val = m_data[m_f];
        m_f = (m_f + 1) % (C + 1);
        return val;
    }
private:
    std::array<T, C+1> m_data;

    usize m_f{0}, m_b{0};
};

}
